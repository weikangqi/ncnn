// Copyright 2020 Tencent
// SPDX-License-Identifier: BSD-3-Clause

static void resize_bicubic_image_pack4_fp16s(const Mat& src, Mat& dst, float* alpha, int* xofs, float* beta, int* yofs)
{
    int w = dst.w;
    int h = dst.h;

    // loop body
    Mat rowsbuf0(w, (size_t)4 * 4u, 4);
    Mat rowsbuf1(w, (size_t)4 * 4u, 4);
    Mat rowsbuf2(w, (size_t)4 * 4u, 4);
    Mat rowsbuf3(w, (size_t)4 * 4u, 4);
    float* rows0 = rowsbuf0;
    float* rows1 = rowsbuf1;
    float* rows2 = rowsbuf2;
    float* rows3 = rowsbuf3;

    int prev_sy1 = -3;

    for (int dy = 0; dy < h; dy++)
    {
        int sy = yofs[dy];

        if (sy == prev_sy1)
        {
            // reuse all rows
        }
        else if (sy == prev_sy1 + 1)
        {
            // hresize one row
            float* rows0_old = rows0;
            rows0 = rows1;
            rows1 = rows2;
            rows2 = rows3;
            rows3 = rows0_old;
            const __fp16* S3 = src.row<const __fp16>(sy + 2);

            const float* alphap = alpha;
            float* rows3p = rows3;
            for (int dx = 0; dx < w; dx++)
            {
                int sx = xofs[dx] * 4;
                const __fp16* S3p = S3 + sx;

                float32x4_t _a0123 = vld1q_f32(alphap);

                float32x4_t _S30 = vcvt_f32_f16(vld1_f16(S3p - 4));
                float32x4_t _S31 = vcvt_f32_f16(vld1_f16(S3p + 0));
                float32x4_t _S32 = vcvt_f32_f16(vld1_f16(S3p + 4));
                float32x4_t _S33 = vcvt_f32_f16(vld1_f16(S3p + 8));
                float32x4_t _rows3 = vmulq_laneq_f32(_S30, _a0123, 0);
                _rows3 = vfmaq_laneq_f32(_rows3, _S31, _a0123, 1);
                _rows3 = vfmaq_laneq_f32(_rows3, _S32, _a0123, 2);
                _rows3 = vfmaq_laneq_f32(_rows3, _S33, _a0123, 3);
                vst1q_f32(rows3p + dx * 4, _rows3);

                alphap += 4;
            }
        }
        else if (sy == prev_sy1 + 2)
        {
            // hresize two rows
            float* rows0_old = rows0;
            float* rows1_old = rows1;
            rows0 = rows2;
            rows1 = rows3;
            rows2 = rows0_old;
            rows3 = rows1_old;
            const __fp16* S2 = src.row<const __fp16>(sy + 1);
            const __fp16* S3 = src.row<const __fp16>(sy + 2);

            const float* alphap = alpha;
            float* rows2p = rows2;
            float* rows3p = rows3;
            for (int dx = 0; dx < w; dx++)
            {
                int sx = xofs[dx] * 4;
                const __fp16* S2p = S2 + sx;
                const __fp16* S3p = S3 + sx;

                float32x4_t _a0123 = vld1q_f32(alphap);

                float32x4_t _S20 = vcvt_f32_f16(vld1_f16(S2p - 4));
                float32x4_t _S21 = vcvt_f32_f16(vld1_f16(S2p + 0));
                float32x4_t _S22 = vcvt_f32_f16(vld1_f16(S2p + 4));
                float32x4_t _S23 = vcvt_f32_f16(vld1_f16(S2p + 8));
                float32x4_t _S30 = vcvt_f32_f16(vld1_f16(S3p - 4));
                float32x4_t _S31 = vcvt_f32_f16(vld1_f16(S3p + 0));
                float32x4_t _S32 = vcvt_f32_f16(vld1_f16(S3p + 4));
                float32x4_t _S33 = vcvt_f32_f16(vld1_f16(S3p + 8));
                float32x4_t _rows2 = vmulq_laneq_f32(_S20, _a0123, 0);
                float32x4_t _rows3 = vmulq_laneq_f32(_S30, _a0123, 0);
                _rows2 = vfmaq_laneq_f32(_rows2, _S21, _a0123, 1);
                _rows3 = vfmaq_laneq_f32(_rows3, _S31, _a0123, 1);
                _rows2 = vfmaq_laneq_f32(_rows2, _S22, _a0123, 2);
                _rows3 = vfmaq_laneq_f32(_rows3, _S32, _a0123, 2);
                _rows2 = vfmaq_laneq_f32(_rows2, _S23, _a0123, 3);
                _rows3 = vfmaq_laneq_f32(_rows3, _S33, _a0123, 3);
                vst1q_f32(rows2p + dx * 4, _rows2);
                vst1q_f32(rows3p + dx * 4, _rows3);

                alphap += 4;
            }
        }
        else if (sy == prev_sy1 + 3)
        {
            // hresize three rows
            float* rows0_old = rows0;
            float* rows1_old = rows1;
            float* rows2_old = rows2;
            rows0 = rows3;
            rows1 = rows0_old;
            rows2 = rows1_old;
            rows3 = rows2_old;
            const __fp16* S1 = src.row<const __fp16>(sy);
            const __fp16* S2 = src.row<const __fp16>(sy + 1);
            const __fp16* S3 = src.row<const __fp16>(sy + 2);

            const float* alphap = alpha;
            float* rows1p = rows1;
            float* rows2p = rows2;
            float* rows3p = rows3;
            for (int dx = 0; dx < w; dx++)
            {
                int sx = xofs[dx] * 4;
                const __fp16* S1p = S1 + sx;
                const __fp16* S2p = S2 + sx;
                const __fp16* S3p = S3 + sx;

                float32x4_t _a0123 = vld1q_f32(alphap);

                float32x4_t _S10 = vcvt_f32_f16(vld1_f16(S1p - 4));
                float32x4_t _S11 = vcvt_f32_f16(vld1_f16(S1p + 0));
                float32x4_t _S12 = vcvt_f32_f16(vld1_f16(S1p + 4));
                float32x4_t _S13 = vcvt_f32_f16(vld1_f16(S1p + 8));
                float32x4_t _S20 = vcvt_f32_f16(vld1_f16(S2p - 4));
                float32x4_t _S21 = vcvt_f32_f16(vld1_f16(S2p + 0));
                float32x4_t _S22 = vcvt_f32_f16(vld1_f16(S2p + 4));
                float32x4_t _S23 = vcvt_f32_f16(vld1_f16(S2p + 8));
                float32x4_t _S30 = vcvt_f32_f16(vld1_f16(S3p - 4));
                float32x4_t _S31 = vcvt_f32_f16(vld1_f16(S3p + 0));
                float32x4_t _S32 = vcvt_f32_f16(vld1_f16(S3p + 4));
                float32x4_t _S33 = vcvt_f32_f16(vld1_f16(S3p + 8));
                float32x4_t _rows1 = vmulq_laneq_f32(_S10, _a0123, 0);
                float32x4_t _rows2 = vmulq_laneq_f32(_S20, _a0123, 0);
                float32x4_t _rows3 = vmulq_laneq_f32(_S30, _a0123, 0);
                _rows1 = vfmaq_laneq_f32(_rows1, _S11, _a0123, 1);
                _rows2 = vfmaq_laneq_f32(_rows2, _S21, _a0123, 1);
                _rows3 = vfmaq_laneq_f32(_rows3, _S31, _a0123, 1);
                _rows1 = vfmaq_laneq_f32(_rows1, _S12, _a0123, 2);
                _rows2 = vfmaq_laneq_f32(_rows2, _S22, _a0123, 2);
                _rows3 = vfmaq_laneq_f32(_rows3, _S32, _a0123, 2);
                _rows1 = vfmaq_laneq_f32(_rows1, _S13, _a0123, 3);
                _rows2 = vfmaq_laneq_f32(_rows2, _S23, _a0123, 3);
                _rows3 = vfmaq_laneq_f32(_rows3, _S33, _a0123, 3);
                vst1q_f32(rows1p + dx * 4, _rows1);
                vst1q_f32(rows2p + dx * 4, _rows2);
                vst1q_f32(rows3p + dx * 4, _rows3);

                alphap += 4;
            }
        }
        else
        {
            // hresize four rows
            const __fp16* S0 = src.row<const __fp16>(sy - 1);
            const __fp16* S1 = src.row<const __fp16>(sy);
            const __fp16* S2 = src.row<const __fp16>(sy + 1);
            const __fp16* S3 = src.row<const __fp16>(sy + 2);

            const float* alphap = alpha;
            float* rows0p = rows0;
            float* rows1p = rows1;
            float* rows2p = rows2;
            float* rows3p = rows3;
            for (int dx = 0; dx < w; dx++)
            {
                int sx = xofs[dx] * 4;
                const __fp16* S0p = S0 + sx;
                const __fp16* S1p = S1 + sx;
                const __fp16* S2p = S2 + sx;
                const __fp16* S3p = S3 + sx;

                float32x4_t _a0123 = vld1q_f32(alphap);

                float32x4_t _S00 = vcvt_f32_f16(vld1_f16(S0p - 4));
                float32x4_t _S01 = vcvt_f32_f16(vld1_f16(S0p + 0));
                float32x4_t _S02 = vcvt_f32_f16(vld1_f16(S0p + 4));
                float32x4_t _S03 = vcvt_f32_f16(vld1_f16(S0p + 8));
                float32x4_t _S10 = vcvt_f32_f16(vld1_f16(S1p - 4));
                float32x4_t _S11 = vcvt_f32_f16(vld1_f16(S1p + 0));
                float32x4_t _S12 = vcvt_f32_f16(vld1_f16(S1p + 4));
                float32x4_t _S13 = vcvt_f32_f16(vld1_f16(S1p + 8));
                float32x4_t _S20 = vcvt_f32_f16(vld1_f16(S2p - 4));
                float32x4_t _S21 = vcvt_f32_f16(vld1_f16(S2p + 0));
                float32x4_t _S22 = vcvt_f32_f16(vld1_f16(S2p + 4));
                float32x4_t _S23 = vcvt_f32_f16(vld1_f16(S2p + 8));
                float32x4_t _S30 = vcvt_f32_f16(vld1_f16(S3p - 4));
                float32x4_t _S31 = vcvt_f32_f16(vld1_f16(S3p + 0));
                float32x4_t _S32 = vcvt_f32_f16(vld1_f16(S3p + 4));
                float32x4_t _S33 = vcvt_f32_f16(vld1_f16(S3p + 8));
                float32x4_t _rows0 = vmulq_laneq_f32(_S00, _a0123, 0);
                float32x4_t _rows1 = vmulq_laneq_f32(_S10, _a0123, 0);
                float32x4_t _rows2 = vmulq_laneq_f32(_S20, _a0123, 0);
                float32x4_t _rows3 = vmulq_laneq_f32(_S30, _a0123, 0);
                _rows0 = vfmaq_laneq_f32(_rows0, _S01, _a0123, 1);
                _rows1 = vfmaq_laneq_f32(_rows1, _S11, _a0123, 1);
                _rows2 = vfmaq_laneq_f32(_rows2, _S21, _a0123, 1);
                _rows3 = vfmaq_laneq_f32(_rows3, _S31, _a0123, 1);
                _rows0 = vfmaq_laneq_f32(_rows0, _S02, _a0123, 2);
                _rows1 = vfmaq_laneq_f32(_rows1, _S12, _a0123, 2);
                _rows2 = vfmaq_laneq_f32(_rows2, _S22, _a0123, 2);
                _rows3 = vfmaq_laneq_f32(_rows3, _S32, _a0123, 2);
                _rows0 = vfmaq_laneq_f32(_rows0, _S03, _a0123, 3);
                _rows1 = vfmaq_laneq_f32(_rows1, _S13, _a0123, 3);
                _rows2 = vfmaq_laneq_f32(_rows2, _S23, _a0123, 3);
                _rows3 = vfmaq_laneq_f32(_rows3, _S33, _a0123, 3);
                vst1q_f32(rows0p + dx * 4, _rows0);
                vst1q_f32(rows1p + dx * 4, _rows1);
                vst1q_f32(rows2p + dx * 4, _rows2);
                vst1q_f32(rows3p + dx * 4, _rows3);

                alphap += 4;
            }
        }

        prev_sy1 = sy;

        // vresize
        float32x4_t _b0123 = vld1q_f32(beta);

        float* rows0p = rows0;
        float* rows1p = rows1;
        float* rows2p = rows2;
        float* rows3p = rows3;
        __fp16* Dp = dst.row<__fp16>(dy);

        for (int dx = 0; dx < w; dx++)
        {
            float32x4_t _rows0 = vld1q_f32(rows0p);
            float32x4_t _rows1 = vld1q_f32(rows1p);
            float32x4_t _rows2 = vld1q_f32(rows2p);
            float32x4_t _rows3 = vld1q_f32(rows3p);
            float32x4_t _Dp = vmulq_laneq_f32(_rows0, _b0123, 0);
            _Dp = vfmaq_laneq_f32(_Dp, _rows1, _b0123, 1);
            _Dp = vfmaq_laneq_f32(_Dp, _rows2, _b0123, 2);
            _Dp = vfmaq_laneq_f32(_Dp, _rows3, _b0123, 3);
            vst1_f16(Dp, vcvt_f16_f32(_Dp));

            Dp += 4;
            rows0p += 4;
            rows1p += 4;
            rows2p += 4;
            rows3p += 4;
        }

        beta += 4;
    }
}

static void resize_bicubic_image_pack4_fp16sa(const Mat& src, Mat& dst, __fp16* alpha, int* xofs, __fp16* beta, int* yofs)
{
    int w = dst.w;
    int h = dst.h;

    // loop body
    Mat rowsbuf0(w, (size_t)4 * 2u, 4);
    Mat rowsbuf1(w, (size_t)4 * 2u, 4);
    Mat rowsbuf2(w, (size_t)4 * 2u, 4);
    Mat rowsbuf3(w, (size_t)4 * 2u, 4);
    __fp16* rows0 = rowsbuf0;
    __fp16* rows1 = rowsbuf1;
    __fp16* rows2 = rowsbuf2;
    __fp16* rows3 = rowsbuf3;

    int prev_sy1 = -3;

    for (int dy = 0; dy < h; dy++)
    {
        int sy = yofs[dy];

        if (sy == prev_sy1)
        {
            // reuse all rows
        }
        else if (sy == prev_sy1 + 1)
        {
            // hresize one row
            __fp16* rows0_old = rows0;
            rows0 = rows1;
            rows1 = rows2;
            rows2 = rows3;
            rows3 = rows0_old;
            const __fp16* S3 = src.row<const __fp16>(sy + 2);

            const __fp16* alphap = alpha;
            __fp16* rows3p = rows3;
            for (int dx = 0; dx < w; dx++)
            {
                int sx = xofs[dx] * 4;
                const __fp16* S3p = S3 + sx;

                float16x4_t _a0123 = vld1_f16(alphap);

                float16x4_t _S30 = vld1_f16(S3p - 4);
                float16x4_t _S31 = vld1_f16(S3p + 0);
                float16x4_t _S32 = vld1_f16(S3p + 4);
                float16x4_t _S33 = vld1_f16(S3p + 8);
                float16x4_t _rows3 = vmul_lane_f16(_S30, _a0123, 0);
                _rows3 = vfma_lane_f16(_rows3, _S31, _a0123, 1);
                _rows3 = vfma_lane_f16(_rows3, _S32, _a0123, 2);
                _rows3 = vfma_lane_f16(_rows3, _S33, _a0123, 3);
                vst1_f16(rows3p + dx * 4, _rows3);

                alphap += 4;
            }
        }
        else if (sy == prev_sy1 + 2)
        {
            // hresize two rows
            __fp16* rows0_old = rows0;
            __fp16* rows1_old = rows1;
            rows0 = rows2;
            rows1 = rows3;
            rows2 = rows0_old;
            rows3 = rows1_old;
            const __fp16* S2 = src.row<const __fp16>(sy + 1);
            const __fp16* S3 = src.row<const __fp16>(sy + 2);

            const __fp16* alphap = alpha;
            __fp16* rows2p = rows2;
            __fp16* rows3p = rows3;
            for (int dx = 0; dx < w; dx++)
            {
                int sx = xofs[dx] * 4;
                const __fp16* S2p = S2 + sx;
                const __fp16* S3p = S3 + sx;

                float16x4_t _a0123 = vld1_f16(alphap);

                float16x4_t _S20 = vld1_f16(S2p - 4);
                float16x4_t _S21 = vld1_f16(S2p + 0);
                float16x4_t _S22 = vld1_f16(S2p + 4);
                float16x4_t _S23 = vld1_f16(S2p + 8);
                float16x4_t _S30 = vld1_f16(S3p - 4);
                float16x4_t _S31 = vld1_f16(S3p + 0);
                float16x4_t _S32 = vld1_f16(S3p + 4);
                float16x4_t _S33 = vld1_f16(S3p + 8);
                float16x4_t _rows2 = vmul_lane_f16(_S20, _a0123, 0);
                float16x4_t _rows3 = vmul_lane_f16(_S30, _a0123, 0);
                _rows2 = vfma_lane_f16(_rows2, _S21, _a0123, 1);
                _rows3 = vfma_lane_f16(_rows3, _S31, _a0123, 1);
                _rows2 = vfma_lane_f16(_rows2, _S22, _a0123, 2);
                _rows3 = vfma_lane_f16(_rows3, _S32, _a0123, 2);
                _rows2 = vfma_lane_f16(_rows2, _S23, _a0123, 3);
                _rows3 = vfma_lane_f16(_rows3, _S33, _a0123, 3);
                vst1_f16(rows2p + dx * 4, _rows2);
                vst1_f16(rows3p + dx * 4, _rows3);

                alphap += 4;
            }
        }
        else if (sy == prev_sy1 + 3)
        {
            // hresize three rows
            __fp16* rows0_old = rows0;
            __fp16* rows1_old = rows1;
            __fp16* rows2_old = rows2;
            rows0 = rows3;
            rows1 = rows0_old;
            rows2 = rows1_old;
            rows3 = rows2_old;
            const __fp16* S1 = src.row<const __fp16>(sy);
            const __fp16* S2 = src.row<const __fp16>(sy + 1);
            const __fp16* S3 = src.row<const __fp16>(sy + 2);

            const __fp16* alphap = alpha;
            __fp16* rows1p = rows1;
            __fp16* rows2p = rows2;
            __fp16* rows3p = rows3;
            for (int dx = 0; dx < w; dx++)
            {
                int sx = xofs[dx] * 4;
                const __fp16* S1p = S1 + sx;
                const __fp16* S2p = S2 + sx;
                const __fp16* S3p = S3 + sx;

                float16x4_t _a0123 = vld1_f16(alphap);

                float16x4_t _S10 = vld1_f16(S1p - 4);
                float16x4_t _S11 = vld1_f16(S1p + 0);
                float16x4_t _S12 = vld1_f16(S1p + 4);
                float16x4_t _S13 = vld1_f16(S1p + 8);
                float16x4_t _S20 = vld1_f16(S2p - 4);
                float16x4_t _S21 = vld1_f16(S2p + 0);
                float16x4_t _S22 = vld1_f16(S2p + 4);
                float16x4_t _S23 = vld1_f16(S2p + 8);
                float16x4_t _S30 = vld1_f16(S3p - 4);
                float16x4_t _S31 = vld1_f16(S3p + 0);
                float16x4_t _S32 = vld1_f16(S3p + 4);
                float16x4_t _S33 = vld1_f16(S3p + 8);
                float16x4_t _rows1 = vmul_lane_f16(_S10, _a0123, 0);
                float16x4_t _rows2 = vmul_lane_f16(_S20, _a0123, 0);
                float16x4_t _rows3 = vmul_lane_f16(_S30, _a0123, 0);
                _rows1 = vfma_lane_f16(_rows1, _S11, _a0123, 1);
                _rows2 = vfma_lane_f16(_rows2, _S21, _a0123, 1);
                _rows3 = vfma_lane_f16(_rows3, _S31, _a0123, 1);
                _rows1 = vfma_lane_f16(_rows1, _S12, _a0123, 2);
                _rows2 = vfma_lane_f16(_rows2, _S22, _a0123, 2);
                _rows3 = vfma_lane_f16(_rows3, _S32, _a0123, 2);
                _rows1 = vfma_lane_f16(_rows1, _S13, _a0123, 3);
                _rows2 = vfma_lane_f16(_rows2, _S23, _a0123, 3);
                _rows3 = vfma_lane_f16(_rows3, _S33, _a0123, 3);
                vst1_f16(rows1p + dx * 4, _rows1);
                vst1_f16(rows2p + dx * 4, _rows2);
                vst1_f16(rows3p + dx * 4, _rows3);

                alphap += 4;
            }
        }
        else
        {
            // hresize four rows
            const __fp16* S0 = src.row<const __fp16>(sy - 1);
            const __fp16* S1 = src.row<const __fp16>(sy);
            const __fp16* S2 = src.row<const __fp16>(sy + 1);
            const __fp16* S3 = src.row<const __fp16>(sy + 2);

            const __fp16* alphap = alpha;
            __fp16* rows0p = rows0;
            __fp16* rows1p = rows1;
            __fp16* rows2p = rows2;
            __fp16* rows3p = rows3;
            for (int dx = 0; dx < w; dx++)
            {
                int sx = xofs[dx] * 4;
                const __fp16* S0p = S0 + sx;
                const __fp16* S1p = S1 + sx;
                const __fp16* S2p = S2 + sx;
                const __fp16* S3p = S3 + sx;

                float16x4_t _a0123 = vld1_f16(alphap);

                float16x4_t _S00 = vld1_f16(S0p - 4);
                float16x4_t _S01 = vld1_f16(S0p + 0);
                float16x4_t _S02 = vld1_f16(S0p + 4);
                float16x4_t _S03 = vld1_f16(S0p + 8);
                float16x4_t _S10 = vld1_f16(S1p - 4);
                float16x4_t _S11 = vld1_f16(S1p + 0);
                float16x4_t _S12 = vld1_f16(S1p + 4);
                float16x4_t _S13 = vld1_f16(S1p + 8);
                float16x4_t _S20 = vld1_f16(S2p - 4);
                float16x4_t _S21 = vld1_f16(S2p + 0);
                float16x4_t _S22 = vld1_f16(S2p + 4);
                float16x4_t _S23 = vld1_f16(S2p + 8);
                float16x4_t _S30 = vld1_f16(S3p - 4);
                float16x4_t _S31 = vld1_f16(S3p + 0);
                float16x4_t _S32 = vld1_f16(S3p + 4);
                float16x4_t _S33 = vld1_f16(S3p + 8);
                float16x4_t _rows0 = vmul_lane_f16(_S00, _a0123, 0);
                float16x4_t _rows1 = vmul_lane_f16(_S10, _a0123, 0);
                float16x4_t _rows2 = vmul_lane_f16(_S20, _a0123, 0);
                float16x4_t _rows3 = vmul_lane_f16(_S30, _a0123, 0);
                _rows0 = vfma_lane_f16(_rows0, _S01, _a0123, 1);
                _rows1 = vfma_lane_f16(_rows1, _S11, _a0123, 1);
                _rows2 = vfma_lane_f16(_rows2, _S21, _a0123, 1);
                _rows3 = vfma_lane_f16(_rows3, _S31, _a0123, 1);
                _rows0 = vfma_lane_f16(_rows0, _S02, _a0123, 2);
                _rows1 = vfma_lane_f16(_rows1, _S12, _a0123, 2);
                _rows2 = vfma_lane_f16(_rows2, _S22, _a0123, 2);
                _rows3 = vfma_lane_f16(_rows3, _S32, _a0123, 2);
                _rows0 = vfma_lane_f16(_rows0, _S03, _a0123, 3);
                _rows1 = vfma_lane_f16(_rows1, _S13, _a0123, 3);
                _rows2 = vfma_lane_f16(_rows2, _S23, _a0123, 3);
                _rows3 = vfma_lane_f16(_rows3, _S33, _a0123, 3);
                vst1_f16(rows0p + dx * 4, _rows0);
                vst1_f16(rows1p + dx * 4, _rows1);
                vst1_f16(rows2p + dx * 4, _rows2);
                vst1_f16(rows3p + dx * 4, _rows3);

                alphap += 4;
            }
        }

        prev_sy1 = sy;

        // vresize
        float16x4_t _b0123 = vld1_f16(beta);

        __fp16* rows0p = rows0;
        __fp16* rows1p = rows1;
        __fp16* rows2p = rows2;
        __fp16* rows3p = rows3;
        __fp16* Dp = dst.row<__fp16>(dy);

        for (int dx = 0; dx < w; dx++)
        {
            float16x4_t _rows0 = vld1_f16(rows0p);
            float16x4_t _rows1 = vld1_f16(rows1p);
            float16x4_t _rows2 = vld1_f16(rows2p);
            float16x4_t _rows3 = vld1_f16(rows3p);
            float16x4_t _Dp = vmul_lane_f16(_rows0, _b0123, 0);
            _Dp = vfma_lane_f16(_Dp, _rows1, _b0123, 1);
            _Dp = vfma_lane_f16(_Dp, _rows2, _b0123, 2);
            _Dp = vfma_lane_f16(_Dp, _rows3, _b0123, 3);
            vst1_f16(Dp, _Dp);

            Dp += 4;
            rows0p += 4;
            rows1p += 4;
            rows2p += 4;
            rows3p += 4;
        }

        beta += 4;
    }
}
