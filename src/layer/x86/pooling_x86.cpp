// Copyright 2017 Tencent
// SPDX-License-Identifier: BSD-3-Clause

#include "pooling_x86.h"

#if __SSE2__
#include <emmintrin.h>
#if __AVX__
#include <immintrin.h>
#endif
#endif // __SSE2__

#include <float.h>

namespace ncnn {

#if __SSE2__
#include "pooling_2x2_pack4.h"
#include "pooling_3x3_pack4.h"
#if __AVX__
#include "pooling_2x2.h"
#include "pooling_2x2_pack8.h"
#include "pooling_3x3_pack8.h"
#if __AVX512F__
#include "pooling_2x2_pack16.h"
#include "pooling_3x3_pack16.h"
#endif // __AVX512F__
#endif // __AVX__
#endif // __SSE2__

Pooling_x86::Pooling_x86()
{
#if __SSE2__
    support_packing = true;
#endif // __SSE2__
}

int Pooling_x86::create_pipeline(const Option& /*opt*/)
{
    if (adaptive_pooling)
    {
        support_packing = false;

        support_bf16_storage = false;
        support_fp16_storage = false;
        support_int8_storage = false;
        support_tensor_storage = false;
    }
    return 0;
}

int Pooling_x86::forward(const Mat& bottom_blob, Mat& top_blob, const Option& opt) const
{
    // max value in NxN window
    // avg value in NxN window

    if (adaptive_pooling)
    {
        return Pooling::forward(bottom_blob, top_blob, opt);
    }

#if __SSE2__
    int elempack = bottom_blob.elempack;
    int w = bottom_blob.w;
    int h = bottom_blob.h;
    int channels = bottom_blob.c;
    size_t elemsize = bottom_blob.elemsize;
#if __AVX__

#if __AVX512F__
    if (elempack == 16)
    {
        if (global_pooling)
        {
            top_blob.create(channels, elemsize, elempack, opt.blob_allocator);
            if (top_blob.empty())
                return -100;

            int size = w * h;

            if (pooling_type == PoolMethod_MAX)
            {
                #pragma omp parallel for num_threads(opt.num_threads)
                for (int q = 0; q < channels; q++)
                {
                    const float* ptr = bottom_blob.channel(q);

                    __m512 _max = _mm512_loadu_ps(ptr);
                    for (int i = 0; i < size; i++)
                    {
                        __m512 _val = _mm512_loadu_ps(ptr);
                        _max = _mm512_max_ps(_max, _val);
                        ptr += 16;
                    }

                    float* outptr = top_blob;
                    _mm512_storeu_ps(outptr + q * 16, _max);
                }
            }
            else if (pooling_type == PoolMethod_AVE)
            {
                #pragma omp parallel for num_threads(opt.num_threads)
                for (int q = 0; q < channels; q++)
                {
                    const float* ptr = bottom_blob.channel(q);

                    __m512 _sum = _mm512_set1_ps(0.f);
                    for (int i = 0; i < size; i++)
                    {
                        __m512 _val = _mm512_loadu_ps(ptr);
                        _sum = _mm512_add_ps(_sum, _val);
                        ptr += 16;
                    }

                    __m512 _inv_size = _mm512_set1_ps(1.f / size);
                    __m512 _avg = _mm512_mul_ps(_sum, _inv_size);

                    float* outptr = top_blob;
                    _mm512_storeu_ps(outptr + q * 16, _avg);
                }
            }

            return 0;
        }

        Mat bottom_blob_bordered;
        make_padding(bottom_blob, bottom_blob_bordered, opt);
        if (bottom_blob_bordered.empty())
            return -100;

        w = bottom_blob_bordered.w;
        h = bottom_blob_bordered.h;

        int outw = (w - kernel_w) / stride_w + 1;
        int outh = (h - kernel_h) / stride_h + 1;

        top_blob.create(outw, outh, channels, elemsize, elempack, opt.blob_allocator);
        if (top_blob.empty())
            return -100;

        const int maxk = kernel_w * kernel_h;

        // kernel offsets
        std::vector<int> _space_ofs(maxk);
        int* space_ofs = &_space_ofs[0];
        {
            int p1 = 0;
            int p2 = 0;
            int gap = w - kernel_w;
            for (int i = 0; i < kernel_h; i++)
            {
                for (int j = 0; j < kernel_w; j++)
                {
                    space_ofs[p1] = p2;
                    p1++;
                    p2++;
                }
                p2 += gap;
            }
        }
        if (pooling_type == PoolMethod_MAX)
        {
            if (kernel_w == 2 && kernel_h == 2 && stride_w == 2 && stride_h == 2)
            {
                pooling2x2s2_max_pack16_avx512(bottom_blob_bordered, top_blob, opt);

                return 0;
            }
            if (kernel_w == 3 && kernel_h == 3 && stride_w == 2 && stride_h == 2)
            {
                pooling3x3s2_max_pack16_avx512(bottom_blob_bordered, top_blob, opt);

                return 0;
            }

            #pragma omp parallel for num_threads(opt.num_threads)
            for (int q = 0; q < channels; q++)
            {
                const Mat m = bottom_blob_bordered.channel(q);
                float* outptr = top_blob.channel(q);

                for (int i = 0; i < outh; i++)
                {
                    for (int j = 0; j < outw; j++)
                    {
                        const float* sptr = m.row(i * stride_h) + j * stride_w * 16;

                        __m512 _max = _mm512_loadu_ps(sptr);

                        for (int k = 0; k < maxk; k++)
                        {
                            __m512 _val = _mm512_loadu_ps(sptr + space_ofs[k] * 16);
                            _max = _mm512_max_ps(_max, _val);
                        }

                        _mm512_storeu_ps(outptr, _max);
                        outptr += 16;
                    }
                }
            }
        }
        else if (pooling_type == PoolMethod_AVE)
        {
            if (avgpool_count_include_pad == 0)
            {
                int wtailpad = 0;
                int htailpad = 0;

                if (pad_mode == 0) // full padding
                {
                    wtailpad = bottom_blob_bordered.w - bottom_blob.w - pad_left - pad_right;
                    htailpad = bottom_blob_bordered.h - bottom_blob.h - pad_top - pad_bottom;
                }

                #pragma omp parallel for num_threads(opt.num_threads)
                for (int q = 0; q < channels; q++)
                {
                    const Mat m = bottom_blob_bordered.channel(q);
                    float* outptr = top_blob.channel(q);

                    for (int i = 0; i < outh; i++)
                    {
                        int sy0 = i * stride_h;

                        for (int j = 0; j < outw; j++)
                        {
                            int sx0 = j * stride_w;

                            __m512 _sum = _mm512_set1_ps(0.f);
                            int area = 0;

                            for (int ki = 0; ki < kernel_h; ki++)
                            {
                                int sy = sy0 + ki;

                                if (sy < pad_top)
                                    continue;

                                if (sy >= h - pad_bottom - htailpad)
                                    break;

                                for (int kj = 0; kj < kernel_w; kj++)
                                {
                                    int sx = sx0 + kj;

                                    if (sx < pad_left)
                                        continue;

                                    if (sx >= w - pad_right - wtailpad)
                                        break;

                                    __m512 _val = _mm512_loadu_ps(m.row(sy) + sx * 16);
                                    _sum = _mm512_add_ps(_sum, _val);
                                    area += 1;
                                }
                            }

                            __m512 _inv_area = _mm512_set1_ps(1.f / area);
                            __m512 _avg = _mm512_mul_ps(_sum, _inv_area);
                            _mm512_storeu_ps(outptr, _avg);
                            outptr += 16;
                        }
                    }
                }
            }
            else // if (avgpool_count_include_pad == 1)
            {
                #pragma omp parallel for num_threads(opt.num_threads)
                for (int q = 0; q < channels; q++)
                {
                    const Mat m = bottom_blob_bordered.channel(q);
                    float* outptr = top_blob.channel(q);

                    __m512 _inv_maxk = _mm512_set1_ps(1.f / maxk);

                    for (int i = 0; i < outh; i++)
                    {
                        for (int j = 0; j < outw; j++)
                        {
                            const float* sptr = m.row(i * stride_h) + j * stride_w * 16;

                            __m512 _sum = _mm512_set1_ps(0.f);

                            for (int k = 0; k < maxk; k++)
                            {
                                __m512 _val = _mm512_loadu_ps(sptr + space_ofs[k] * 16);
                                _sum = _mm512_add_ps(_sum, _val);
                            }

                            __m512 _avg = _mm512_mul_ps(_sum, _inv_maxk);
                            _mm512_storeu_ps(outptr, _avg);
                            outptr += 16;
                        }
                    }
                }
            }
        }

        return 0;
    }
#endif // __AVX512F__

    //     NCNN_LOGE("Pooling     input %d x %d  pad = %d %d %d %d  ksize=%d %d  stride=%d %d", w, h, pad_left, pad_right, pad_top, pad_bottom, kernel_w, kernel_h, stride_w, stride_h);
    if (elempack == 8)
    {
        if (global_pooling)
        {
            top_blob.create(channels, elemsize, elempack, opt.blob_allocator);
            if (top_blob.empty())
                return -100;

            int size = w * h;

            if (pooling_type == PoolMethod_MAX)
            {
                #pragma omp parallel for num_threads(opt.num_threads)
                for (int q = 0; q < channels; q++)
                {
                    const float* ptr = bottom_blob.channel(q);

                    __m256 _max = _mm256_loadu_ps(ptr);
                    for (int i = 0; i < size; i++)
                    {
                        __m256 _val = _mm256_loadu_ps(ptr);
                        _max = _mm256_max_ps(_max, _val);
                        ptr += 8;
                    }

                    float* outptr = top_blob;
                    _mm256_storeu_ps(outptr + q * 8, _max);
                }
            }
            else if (pooling_type == PoolMethod_AVE)
            {
                #pragma omp parallel for num_threads(opt.num_threads)
                for (int q = 0; q < channels; q++)
                {
                    const float* ptr = bottom_blob.channel(q);

                    __m256 _sum = _mm256_set1_ps(0.f);
                    for (int i = 0; i < size; i++)
                    {
                        __m256 _val = _mm256_loadu_ps(ptr);
                        _sum = _mm256_add_ps(_sum, _val);
                        ptr += 8;
                    }

                    __m256 _inv_size = _mm256_set1_ps(1.f / size);
                    __m256 _avg = _mm256_mul_ps(_sum, _inv_size);

                    float* outptr = top_blob;
                    _mm256_storeu_ps(outptr + q * 8, _avg);
                }
            }

            return 0;
        }

        Mat bottom_blob_bordered;
        make_padding(bottom_blob, bottom_blob_bordered, opt);
        if (bottom_blob_bordered.empty())
            return -100;

        w = bottom_blob_bordered.w;
        h = bottom_blob_bordered.h;

        int outw = (w - kernel_w) / stride_w + 1;
        int outh = (h - kernel_h) / stride_h + 1;

        top_blob.create(outw, outh, channels, elemsize, elempack, opt.blob_allocator);
        if (top_blob.empty())
            return -100;

        const int maxk = kernel_w * kernel_h;

        // kernel offsets
        std::vector<int> _space_ofs(maxk);
        int* space_ofs = &_space_ofs[0];
        {
            int p1 = 0;
            int p2 = 0;
            int gap = w - kernel_w;
            for (int i = 0; i < kernel_h; i++)
            {
                for (int j = 0; j < kernel_w; j++)
                {
                    space_ofs[p1] = p2;
                    p1++;
                    p2++;
                }
                p2 += gap;
            }
        }
        if (pooling_type == PoolMethod_MAX)
        {
            if (kernel_w == 2 && kernel_h == 2 && stride_w == 2 && stride_h == 2)
            {
                pooling2x2s2_max_pack8_avx(bottom_blob_bordered, top_blob, opt);

                return 0;
            }
            if (kernel_w == 3 && kernel_h == 3 && stride_w == 2 && stride_h == 2)
            {
                pooling3x3s2_max_pack8_avx(bottom_blob_bordered, top_blob, opt);

                return 0;
            }

            #pragma omp parallel for num_threads(opt.num_threads)
            for (int q = 0; q < channels; q++)
            {
                const Mat m = bottom_blob_bordered.channel(q);
                float* outptr = top_blob.channel(q);

                for (int i = 0; i < outh; i++)
                {
                    for (int j = 0; j < outw; j++)
                    {
                        const float* sptr = m.row(i * stride_h) + j * stride_w * 8;

                        __m256 _max = _mm256_loadu_ps(sptr);

                        for (int k = 0; k < maxk; k++)
                        {
                            __m256 _val = _mm256_loadu_ps(sptr + space_ofs[k] * 8);
                            _max = _mm256_max_ps(_max, _val);
                        }

                        _mm256_storeu_ps(outptr + j * 8, _max);
                    }

                    outptr += outw * 8;
                }
            }
        }
        else if (pooling_type == PoolMethod_AVE)
        {
            if (avgpool_count_include_pad == 0)
            {
                int wtailpad = 0;
                int htailpad = 0;

                if (pad_mode == 0) // full padding
                {
                    wtailpad = bottom_blob_bordered.w - bottom_blob.w - pad_left - pad_right;
                    htailpad = bottom_blob_bordered.h - bottom_blob.h - pad_top - pad_bottom;
                }

                #pragma omp parallel for num_threads(opt.num_threads)
                for (int q = 0; q < channels; q++)
                {
                    const Mat m = bottom_blob_bordered.channel(q);
                    float* outptr = top_blob.channel(q);

                    for (int i = 0; i < outh; i++)
                    {
                        int sy0 = i * stride_h;

                        for (int j = 0; j < outw; j++)
                        {
                            int sx0 = j * stride_w;

                            __m256 _sum = _mm256_set1_ps(0.f);
                            int area = 0;

                            for (int ki = 0; ki < kernel_h; ki++)
                            {
                                int sy = sy0 + ki;

                                if (sy < pad_top)
                                    continue;

                                if (sy >= h - pad_bottom - htailpad)
                                    break;

                                for (int kj = 0; kj < kernel_w; kj++)
                                {
                                    int sx = sx0 + kj;

                                    if (sx < pad_left)
                                        continue;

                                    if (sx >= w - pad_right - wtailpad)
                                        break;

                                    __m256 _val = _mm256_loadu_ps(m.row(sy) + sx * 8);
                                    _sum = _mm256_add_ps(_sum, _val);
                                    area += 1;
                                }
                            }

                            __m256 _inv_area = _mm256_set1_ps(1.f / area);
                            __m256 _avg = _mm256_mul_ps(_sum, _inv_area);
                            _mm256_storeu_ps(outptr + j * 8, _avg);
                        }

                        outptr += outw * 8;
                    }
                }
            }
            else // if (avgpool_count_include_pad == 1)
            {
                #pragma omp parallel for num_threads(opt.num_threads)
                for (int q = 0; q < channels; q++)
                {
                    const Mat m = bottom_blob_bordered.channel(q);
                    float* outptr = top_blob.channel(q);

                    __m256 _inv_maxk = _mm256_set1_ps(1.f / maxk);

                    for (int i = 0; i < outh; i++)
                    {
                        for (int j = 0; j < outw; j++)
                        {
                            const float* sptr = m.row(i * stride_h) + j * stride_w * 8;

                            __m256 _sum = _mm256_set1_ps(0.f);

                            for (int k = 0; k < maxk; k++)
                            {
                                __m256 _val = _mm256_loadu_ps(sptr + space_ofs[k] * 8);
                                _sum = _mm256_add_ps(_sum, _val);
                            }

                            __m256 _avg = _mm256_mul_ps(_sum, _inv_maxk);
                            _mm256_storeu_ps(outptr + j * 8, _avg);
                        }

                        outptr += outw * 8;
                    }
                }
            }
        }

        return 0;
    }
#endif // __AVX__

    if (elempack == 4)
    {
        if (global_pooling)
        {
            top_blob.create(channels, elemsize, elempack, opt.blob_allocator);
            if (top_blob.empty())
                return -100;

            int size = w * h;

            if (pooling_type == PoolMethod_MAX)
            {
                #pragma omp parallel for num_threads(opt.num_threads)
                for (int q = 0; q < channels; q++)
                {
                    const float* ptr = bottom_blob.channel(q);

                    __m128 _max = _mm_loadu_ps(ptr);
                    for (int i = 0; i < size; i++)
                    {
                        __m128 _val = _mm_loadu_ps(ptr);
                        _max = _mm_max_ps(_max, _val);
                        ptr += 4;
                    }

                    float* outptr = top_blob;
                    _mm_storeu_ps(outptr + q * 4, _max);
                }
            }
            else if (pooling_type == PoolMethod_AVE)
            {
                #pragma omp parallel for num_threads(opt.num_threads)
                for (int q = 0; q < channels; q++)
                {
                    const float* ptr = bottom_blob.channel(q);

                    __m128 _sum = _mm_set1_ps(0.f);
                    for (int i = 0; i < size; i++)
                    {
                        __m128 _val = _mm_loadu_ps(ptr);
                        _sum = _mm_add_ps(_sum, _val);
                        ptr += 4;
                    }

                    __m128 _inv_size = _mm_set1_ps(1.f / size);
                    __m128 _avg = _mm_mul_ps(_sum, _inv_size);

                    float* outptr = top_blob;
                    _mm_storeu_ps(outptr + q * 4, _avg);
                }
            }

            return 0;
        }

        Mat bottom_blob_bordered;
        make_padding(bottom_blob, bottom_blob_bordered, opt);
        if (bottom_blob_bordered.empty())
            return -100;

        w = bottom_blob_bordered.w;
        h = bottom_blob_bordered.h;

        int outw = (w - kernel_w) / stride_w + 1;
        int outh = (h - kernel_h) / stride_h + 1;

        top_blob.create(outw, outh, channels, elemsize, elempack, opt.blob_allocator);
        if (top_blob.empty())
            return -100;

        const int maxk = kernel_w * kernel_h;

        // kernel offsets
        std::vector<int> _space_ofs(maxk);
        int* space_ofs = &_space_ofs[0];
        {
            int p1 = 0;
            int p2 = 0;
            int gap = w - kernel_w;
            for (int i = 0; i < kernel_h; i++)
            {
                for (int j = 0; j < kernel_w; j++)
                {
                    space_ofs[p1] = p2;
                    p1++;
                    p2++;
                }
                p2 += gap;
            }
        }
        if (pooling_type == PoolMethod_MAX)
        {
            if (kernel_w == 2 && kernel_h == 2 && stride_w == 2 && stride_h == 2)
            {
                pooling2x2s2_max_pack4_sse(bottom_blob_bordered, top_blob, opt);

                return 0;
            }
            if (kernel_w == 3 && kernel_h == 3 && stride_w == 2 && stride_h == 2)
            {
                pooling3x3s2_max_pack4_sse(bottom_blob_bordered, top_blob, opt);

                return 0;
            }

            #pragma omp parallel for num_threads(opt.num_threads)
            for (int q = 0; q < channels; q++)
            {
                const Mat m = bottom_blob_bordered.channel(q);
                float* outptr = top_blob.channel(q);

                for (int i = 0; i < outh; i++)
                {
                    for (int j = 0; j < outw; j++)
                    {
                        const float* sptr = m.row(i * stride_h) + j * stride_w * 4;

                        __m128 _max = _mm_loadu_ps(sptr);

                        for (int k = 0; k < maxk; k++)
                        {
                            __m128 _val = _mm_loadu_ps(sptr + space_ofs[k] * 4);
                            _max = _mm_max_ps(_max, _val);
                        }

                        _mm_storeu_ps(outptr + j * 4, _max);
                    }

                    outptr += outw * 4;
                }
            }
        }
        else if (pooling_type == PoolMethod_AVE)
        {
            if (avgpool_count_include_pad == 0)
            {
                int wtailpad = 0;
                int htailpad = 0;

                if (pad_mode == 0) // full padding
                {
                    wtailpad = bottom_blob_bordered.w - bottom_blob.w - pad_left - pad_right;
                    htailpad = bottom_blob_bordered.h - bottom_blob.h - pad_top - pad_bottom;
                }

                #pragma omp parallel for num_threads(opt.num_threads)
                for (int q = 0; q < channels; q++)
                {
                    const Mat m = bottom_blob_bordered.channel(q);
                    float* outptr = top_blob.channel(q);

                    for (int i = 0; i < outh; i++)
                    {
                        int sy0 = i * stride_h;

                        for (int j = 0; j < outw; j++)
                        {
                            int sx0 = j * stride_w;

                            __m128 _sum = _mm_set1_ps(0.f);
                            int area = 0;

                            for (int ki = 0; ki < kernel_h; ki++)
                            {
                                int sy = sy0 + ki;

                                if (sy < pad_top)
                                    continue;

                                if (sy >= h - pad_bottom - htailpad)
                                    break;

                                for (int kj = 0; kj < kernel_w; kj++)
                                {
                                    int sx = sx0 + kj;

                                    if (sx < pad_left)
                                        continue;

                                    if (sx >= w - pad_right - wtailpad)
                                        break;

                                    __m128 _val = _mm_loadu_ps(m.row(sy) + sx * 4);
                                    _sum = _mm_add_ps(_sum, _val);
                                    area += 1;
                                }
                            }

                            __m128 _inv_area = _mm_set1_ps(1.f / area);
                            __m128 _avg = _mm_mul_ps(_sum, _inv_area);
                            _mm_storeu_ps(outptr + j * 4, _avg);
                        }

                        outptr += outw * 4;
                    }
                }
            }
            else // if (avgpool_count_include_pad == 1)
            {
                #pragma omp parallel for num_threads(opt.num_threads)
                for (int q = 0; q < channels; q++)
                {
                    const Mat m = bottom_blob_bordered.channel(q);
                    float* outptr = top_blob.channel(q);

                    __m128 _inv_maxk = _mm_set1_ps(1.f / maxk);

                    for (int i = 0; i < outh; i++)
                    {
                        for (int j = 0; j < outw; j++)
                        {
                            const float* sptr = m.row(i * stride_h) + j * stride_w * 4;

                            __m128 _sum = _mm_set1_ps(0.f);

                            for (int k = 0; k < maxk; k++)
                            {
                                __m128 _val = _mm_loadu_ps(sptr + space_ofs[k] * 4);
                                _sum = _mm_add_ps(_sum, _val);
                            }

                            __m128 _avg = _mm_mul_ps(_sum, _inv_maxk);
                            _mm_storeu_ps(outptr + j * 4, _avg);
                        }

                        outptr += outw * 4;
                    }
                }
            }
        }

        return 0;
    }
#endif // __SSE2__

    if (kernel_w != kernel_h || stride_w != stride_h)
    {
        return Pooling::forward(bottom_blob, top_blob, opt);
    }

    const int stride = stride_w;

    if (pooling_type != PoolMethod_MAX || stride != 2 || global_pooling == 1)
    {
        return Pooling::forward(bottom_blob, top_blob, opt);
    }

#if __AVX__
    const int kernel_size = kernel_w;

    if (kernel_size != 2)
    {
        return Pooling::forward(bottom_blob, top_blob, opt);
    }

    Mat bottom_blob_bordered;
    make_padding(bottom_blob, bottom_blob_bordered, opt);
    if (bottom_blob_bordered.empty())
        return -100;

    w = bottom_blob_bordered.w;
    h = bottom_blob_bordered.h;

    int outw = (w - kernel_w) / stride_w + 1;
    int outh = (h - kernel_h) / stride_h + 1;

    top_blob.create(outw, outh, channels, elemsize, opt.blob_allocator);
    if (top_blob.empty())
        return -100;

    if (kernel_size == 2)
        pooling2x2s2_max_avx(bottom_blob_bordered, top_blob, opt);

    return 0;
#else
    return Pooling::forward(bottom_blob, top_blob, opt);
#endif
}

} // namespace ncnn
