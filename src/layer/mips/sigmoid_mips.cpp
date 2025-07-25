// Copyright 2020 Leo <leo@nullptr.com.cn>
// SPDX-License-Identifier: BSD-3-Clause

#include "sigmoid_mips.h"

#if __mips_msa
#include <msa.h>
#include "msa_mathfun.h"
#endif // __mips_msa

#include "mips_usability.h"

namespace ncnn {

Sigmoid_mips::Sigmoid_mips()
{
#if __mips_msa
    support_packing = true;
#endif
}

int Sigmoid_mips::forward_inplace(Mat& bottom_top_blob, const Option& opt) const
{
    int w = bottom_top_blob.w;
    int h = bottom_top_blob.h;
    int d = bottom_top_blob.d;
    int channels = bottom_top_blob.c;
    int elempack = bottom_top_blob.elempack;
    int size = w * h * d * elempack;

    #pragma omp parallel for num_threads(opt.num_threads)
    for (int q = 0; q < channels; q++)
    {
        float* ptr = bottom_top_blob.channel(q);

        int i = 0;
#if __mips_msa
        v4f32 _one = (v4f32)__msa_fill_w_f32(1.f);
        for (; i + 3 < size; i += 4)
        {
            __builtin_prefetch(ptr + 16);
            v4f32 _p = (v4f32)__msa_ld_w(ptr, 0);
            _p = (v4f32)__msa_bnegi_w((v4u32)_p, 31);
            _p = exp_ps(_p);
            _p = __msa_fadd_w(_p, _one);
            v4f32 _outp = __msa_fdiv_w(_one, _p);
            __msa_st_w((v4i32)_outp, ptr, 0);

            ptr += 4;
        }
#endif // __mips_msa
        for (; i < size; i++)
        {
            *ptr = 1.f / (1.f + expf(-*ptr));

            ptr++;
        }
    }

    return 0;
}

} // namespace ncnn
