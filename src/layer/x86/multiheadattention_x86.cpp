// Copyright 2023 Tencent
// SPDX-License-Identifier: BSD-3-Clause

#include "multiheadattention_x86.h"

#include "layer_type.h"

namespace ncnn {

MultiHeadAttention_x86::MultiHeadAttention_x86()
{
#if __SSE2__
    support_packing = true;
#endif // __SSE2__

    q_gemm = 0;
    k_gemm = 0;
    v_gemm = 0;

    qk_gemm = 0;
    qkv_gemm = 0;

    qk_softmax = 0;

    o_gemm = 0;
}

int MultiHeadAttention_x86::create_pipeline(const Option& _opt)
{
    Option opt = _opt;
    if (int8_scale_term)
    {
        support_packing = false;

        opt.use_packing_layout = false; // TODO enable packing
    }

    {
        qk_softmax = ncnn::create_layer_cpu(ncnn::LayerType::Softmax);
        ncnn::ParamDict pd;
        pd.set(0, -1);
        pd.set(1, 1);
        qk_softmax->load_param(pd);
        qk_softmax->load_model(ModelBinFromMatArray(0));
        qk_softmax->create_pipeline(opt);
    }

    const int qdim = weight_data_size / embed_dim;

    {
        q_gemm = ncnn::create_layer_cpu(ncnn::LayerType::Gemm);
        ncnn::ParamDict pd;
        pd.set(0, scale);
        pd.set(1, 1.f);
        pd.set(2, 0);         // transA
        pd.set(3, 1);         // transB
        pd.set(4, 1);         // constantA
        pd.set(5, 0);         // constantB
        pd.set(6, 1);         // constantC
        pd.set(7, embed_dim); // M
        pd.set(8, 0);         // N
        pd.set(9, qdim);      // K
        pd.set(10, 1);        // constant_broadcast_type_C
        pd.set(11, 0);        // output_N1M
        pd.set(12, 1);        // output_elempack
        pd.set(14, 0);        // output_transpose
#if NCNN_INT8
        pd.set(18, int8_scale_term);
#endif
        q_gemm->load_param(pd);
        Mat weights[3];
        weights[0] = q_weight_data;
        weights[1] = q_bias_data;
#if NCNN_INT8
        weights[2] = q_weight_data_int8_scales;
#endif
        q_gemm->load_model(ModelBinFromMatArray(weights));
        q_gemm->create_pipeline(opt);

        if (opt.lightmode)
        {
            q_weight_data.release();
            q_bias_data.release();
        }
    }

    {
        k_gemm = ncnn::create_layer_cpu(ncnn::LayerType::Gemm);
        ncnn::ParamDict pd;
        pd.set(2, 0);         // transA
        pd.set(3, 1);         // transB
        pd.set(4, 1);         // constantA
        pd.set(5, 0);         // constantB
        pd.set(6, 1);         // constantC
        pd.set(7, embed_dim); // M
        pd.set(8, 0);         // N
        pd.set(9, kdim);      // K
        pd.set(10, 1);        // constant_broadcast_type_C
        pd.set(11, 0);        // output_N1M
        pd.set(12, 1);        // output_elempack
        pd.set(14, 0);        // output_transpose
#if NCNN_INT8
        pd.set(18, int8_scale_term);
#endif
        k_gemm->load_param(pd);
        Mat weights[3];
        weights[0] = k_weight_data;
        weights[1] = k_bias_data;
#if NCNN_INT8
        weights[2] = k_weight_data_int8_scales;
#endif
        k_gemm->load_model(ModelBinFromMatArray(weights));
        k_gemm->create_pipeline(opt);

        if (opt.lightmode)
        {
            k_weight_data.release();
            k_bias_data.release();
        }
    }

    {
        v_gemm = ncnn::create_layer_cpu(ncnn::LayerType::Gemm);
        ncnn::ParamDict pd;
        pd.set(2, 0);         // transA
        pd.set(3, 1);         // transB
        pd.set(4, 1);         // constantA
        pd.set(5, 0);         // constantB
        pd.set(6, 1);         // constantC
        pd.set(7, embed_dim); // M
        pd.set(8, 0);         // N
        pd.set(9, vdim);      // K
        pd.set(10, 1);        // constant_broadcast_type_C
        pd.set(11, 0);        // output_N1M
        pd.set(12, 1);        // output_elempack
        pd.set(14, 0);        // output_transpose
#if NCNN_INT8
        pd.set(18, int8_scale_term);
#endif
        v_gemm->load_param(pd);
        Mat weights[3];
        weights[0] = v_weight_data;
        weights[1] = v_bias_data;
#if NCNN_INT8
        weights[2] = v_weight_data_int8_scales;
#endif
        v_gemm->load_model(ModelBinFromMatArray(weights));
        v_gemm->create_pipeline(opt);

        if (opt.lightmode)
        {
            v_weight_data.release();
            v_bias_data.release();
        }
    }

    {
        o_gemm = ncnn::create_layer_cpu(ncnn::LayerType::Gemm);
        ncnn::ParamDict pd;
        pd.set(2, 1);         // transA
        pd.set(3, 1);         // transB
        pd.set(4, 0);         // constantA
        pd.set(5, 1);         // constantB
        pd.set(6, 1);         // constantC
        pd.set(7, 0);         // M = outch
        pd.set(8, qdim);      // N = size
        pd.set(9, embed_dim); // K = maxk*inch
        pd.set(10, 4);        // constant_broadcast_type_C
        pd.set(11, 0);        // output_N1M
#if NCNN_INT8
        pd.set(18, int8_scale_term);
#endif
        o_gemm->load_param(pd);
        Mat weights[3];
        weights[0] = out_weight_data;
        weights[1] = out_bias_data;
#if NCNN_INT8
        Mat out_weight_data_int8_scales(1);
        out_weight_data_int8_scales[0] = out_weight_data_int8_scale;
        weights[2] = out_weight_data_int8_scales;
#endif
        o_gemm->load_model(ModelBinFromMatArray(weights));
        o_gemm->create_pipeline(opt);

        if (opt.lightmode)
        {
            out_weight_data.release();
            out_bias_data.release();
        }
    }

    {
        qk_gemm = ncnn::create_layer_cpu(ncnn::LayerType::Gemm);
        ncnn::ParamDict pd;
        pd.set(2, 1);                   // transA
        pd.set(3, 0);                   // transB
        pd.set(4, 0);                   // constantA
        pd.set(5, 0);                   // constantB
        pd.set(6, attn_mask ? 0 : 1);   // constantC
        pd.set(7, 0);                   // M
        pd.set(8, 0);                   // N
        pd.set(9, 0);                   // K
        pd.set(10, attn_mask ? 3 : -1); // constant_broadcast_type_C
        pd.set(11, 0);                  // output_N1M
        pd.set(12, 1);                  // output_elempack
#if NCNN_INT8
        pd.set(18, int8_scale_term);
#endif
        qk_gemm->load_param(pd);
        qk_gemm->load_model(ModelBinFromMatArray(0));
        Option opt1 = opt;
        opt1.num_threads = 1;
        qk_gemm->create_pipeline(opt1);
    }

    {
        qkv_gemm = ncnn::create_layer_cpu(ncnn::LayerType::Gemm);
        ncnn::ParamDict pd;
        pd.set(2, 0);   // transA
        pd.set(3, 1);   // transB
        pd.set(4, 0);   // constantA
        pd.set(5, 0);   // constantB
        pd.set(6, 1);   // constantC
        pd.set(7, 0);   // M
        pd.set(8, 0);   // N
        pd.set(9, 0);   // K
        pd.set(10, -1); // constant_broadcast_type_C
        pd.set(11, 0);  // output_N1M
        pd.set(12, 1);  // output_elempack
        pd.set(14, 1);  // output_transpose
#if NCNN_INT8
        pd.set(18, int8_scale_term);
#endif
        qkv_gemm->load_param(pd);
        qkv_gemm->load_model(ModelBinFromMatArray(0));
        Option opt1 = opt;
        opt1.num_threads = 1;
        qkv_gemm->create_pipeline(opt1);
    }

    return 0;
}

int MultiHeadAttention_x86::destroy_pipeline(const Option& _opt)
{
    Option opt = _opt;
    if (int8_scale_term)
    {
        opt.use_packing_layout = false; // TODO enable packing
    }

    if (qk_softmax)
    {
        qk_softmax->destroy_pipeline(opt);
        delete qk_softmax;
        qk_softmax = 0;
    }

    if (q_gemm)
    {
        q_gemm->destroy_pipeline(opt);
        delete q_gemm;
        q_gemm = 0;
    }

    if (k_gemm)
    {
        k_gemm->destroy_pipeline(opt);
        delete k_gemm;
        k_gemm = 0;
    }

    if (v_gemm)
    {
        v_gemm->destroy_pipeline(opt);
        delete v_gemm;
        v_gemm = 0;
    }

    if (o_gemm)
    {
        o_gemm->destroy_pipeline(opt);
        delete o_gemm;
        o_gemm = 0;
    }

    if (qk_gemm)
    {
        qk_gemm->destroy_pipeline(opt);
        delete qk_gemm;
        qk_gemm = 0;
    }
    if (qkv_gemm)
    {
        qkv_gemm->destroy_pipeline(opt);
        delete qkv_gemm;
        qkv_gemm = 0;
    }

    return 0;
}

int MultiHeadAttention_x86::forward(const std::vector<Mat>& bottom_blobs, std::vector<Mat>& top_blobs, const Option& _opt) const
{
    const Mat& q_blob = bottom_blobs[0];
    const Mat& k_blob = (bottom_blobs.size() == 1 || (bottom_blobs.size() == 2 && attn_mask)) ? q_blob : bottom_blobs[1];
    const Mat& v_blob = (bottom_blobs.size() == 1 || (bottom_blobs.size() == 2 && attn_mask)) ? q_blob : (bottom_blobs.size() == 2 || (bottom_blobs.size() == 3 && attn_mask)) ? k_blob : bottom_blobs[2];
    const Mat& attn_mask_blob = attn_mask ? bottom_blobs[bottom_blobs.size() - 1] : Mat();

    Option opt = _opt;
    if (int8_scale_term)
    {
        opt.use_packing_layout = false; // TODO enable packing
    }

    Mat attn_mask_blob_unpacked;
    if (attn_mask && attn_mask_blob.elempack != 1)
    {
        convert_packing(attn_mask_blob, attn_mask_blob_unpacked, 1, opt);
        if (attn_mask_blob_unpacked.empty())
            return -100;
    }
    else
    {
        attn_mask_blob_unpacked = attn_mask_blob;
    }

    const int embed_dim_per_head = embed_dim / num_heads;
    const int src_seqlen = q_blob.h * q_blob.elempack;
    const int dst_seqlen = k_blob.h * k_blob.elempack;

    Mat q_affine;
    int retq = q_gemm->forward(q_blob, q_affine, opt);
    if (retq != 0)
        return retq;

    Mat k_affine;
    int retk = k_gemm->forward(k_blob, k_affine, opt);
    if (retk != 0)
        return retk;

    Mat qk_cross(dst_seqlen, src_seqlen * num_heads, 4u, opt.blob_allocator);
    if (qk_cross.empty())
        return -100;

    std::vector<int> retqks;
    retqks.resize(num_heads);
    #pragma omp parallel for num_threads(opt.num_threads)
    for (int i = 0; i < num_heads; i++)
    {
        std::vector<Mat> qk_bottom_blobs(2);
        qk_bottom_blobs[0] = q_affine.row_range(i * embed_dim_per_head, embed_dim_per_head);
        qk_bottom_blobs[1] = k_affine.row_range(i * embed_dim_per_head, embed_dim_per_head);
        if (attn_mask)
        {
            const Mat& maskm = attn_mask_blob_unpacked.dims == 3 ? attn_mask_blob_unpacked.channel(i) : attn_mask_blob_unpacked;
            qk_bottom_blobs.push_back(maskm);
        }
        std::vector<Mat> qk_top_blobs(1);
        qk_top_blobs[0] = qk_cross.row_range(i * src_seqlen, src_seqlen);
        Option opt1 = opt;
        opt1.num_threads = 1;
        retqks[i] = qk_gemm->forward(qk_bottom_blobs, qk_top_blobs, opt1);
    }
    for (int i = 0; i < num_heads; i++)
    {
        if (retqks[i] != 0)
            return retqks[i];
    }

    q_affine.release();
    k_affine.release();

    int retqk = qk_softmax->forward_inplace(qk_cross, opt);
    if (retqk != 0)
        return retqk;

    Mat v_affine;
    int retv = v_gemm->forward(v_blob, v_affine, opt);
    if (retv != 0)
        return retv;

    Mat qkv_cross(src_seqlen, embed_dim_per_head * num_heads, 4u, opt.blob_allocator);
    if (qkv_cross.empty())
        return -100;

    std::vector<int> retqkvs;
    retqkvs.resize(num_heads);
    #pragma omp parallel for num_threads(opt.num_threads)
    for (int i = 0; i < num_heads; i++)
    {
        std::vector<Mat> qkv_bottom_blobs(2);
        qkv_bottom_blobs[0] = qk_cross.row_range(i * src_seqlen, src_seqlen);
        qkv_bottom_blobs[1] = v_affine.row_range(i * embed_dim_per_head, embed_dim_per_head);
        std::vector<Mat> qkv_top_blobs(1);
        qkv_top_blobs[0] = qkv_cross.row_range(i * embed_dim_per_head, embed_dim_per_head);
        Option opt1 = opt;
        opt1.num_threads = 1;
        retqkvs[i] = qkv_gemm->forward(qkv_bottom_blobs, qkv_top_blobs, opt1);
    }
    for (int i = 0; i < num_heads; i++)
    {
        if (retqkvs[i] != 0)
            return retqkvs[i];
    }

    v_affine.release();

    int reto = o_gemm->forward(qkv_cross, top_blobs[0], opt);
    if (reto != 0)
        return reto;

    return 0;
}

} // namespace ncnn
