// Copyright 2021 Tencent
// SPDX-License-Identifier: BSD-3-Clause

#include "testutil.h"

static int test_gelu(const ncnn::Mat& a, bool fast_gelu)
{
    ncnn::ParamDict pd;
    pd.set(0, fast_gelu ? 1 : 0);

    std::vector<ncnn::Mat> weights(0);

    int ret = test_layer("GELU", pd, weights, a);
    if (ret != 0)
    {
        fprintf(stderr, "test_gelu failed a.dims=%d a=(%d %d %d %d) fast_gelu=%s\n", a.dims, a.w, a.h, a.d, a.c, fast_gelu ? "true" : "false");
    }

    return ret;
}

static int test_gelu_0()
{
    return 0
           || test_gelu(RandomMat(6, 7, 9, 32), false)
           || test_gelu(RandomMat(6, 7, 9, 32), true)
           || test_gelu(RandomMat(5, 6, 7, 24), false)
           || test_gelu(RandomMat(5, 6, 7, 24), true)
           || test_gelu(RandomMat(7, 8, 9, 12), false)
           || test_gelu(RandomMat(7, 8, 9, 12), true)
           || test_gelu(RandomMat(3, 4, 5, 13), false)
           || test_gelu(RandomMat(3, 4, 5, 13), true);
}

static int test_gelu_1()
{
    return 0
           || test_gelu(RandomMat(9, 7, 32), false)
           || test_gelu(RandomMat(9, 7, 32), true)
           || test_gelu(RandomMat(5, 7, 24), false)
           || test_gelu(RandomMat(5, 7, 24), true)
           || test_gelu(RandomMat(7, 9, 12), false)
           || test_gelu(RandomMat(7, 9, 12), true)
           || test_gelu(RandomMat(3, 5, 13), false)
           || test_gelu(RandomMat(3, 5, 13), true);
}

static int test_gelu_2()
{
    return 0
           || test_gelu(RandomMat(13, 32), false)
           || test_gelu(RandomMat(13, 32), true)
           || test_gelu(RandomMat(15, 24), false)
           || test_gelu(RandomMat(15, 24), true)
           || test_gelu(RandomMat(17, 12), false)
           || test_gelu(RandomMat(17, 12), true)
           || test_gelu(RandomMat(19, 15), false)
           || test_gelu(RandomMat(19, 15), true);
}

static int test_gelu_3()
{
    return 0
           || test_gelu(RandomMat(128), false)
           || test_gelu(RandomMat(128), true)
           || test_gelu(RandomMat(124), false)
           || test_gelu(RandomMat(124), true)
           || test_gelu(RandomMat(127), false)
           || test_gelu(RandomMat(127), true)
           || test_gelu(RandomMat(120), false)
           || test_gelu(RandomMat(120), true);
}

int main()
{
    SRAND(7767517);

    return 0
           || test_gelu_0()
           || test_gelu_1()
           || test_gelu_2()
           || test_gelu_3();
}
