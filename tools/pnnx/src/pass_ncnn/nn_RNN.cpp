// Copyright 2021 Tencent
// SPDX-License-Identifier: BSD-3-Clause

#include "pass_ncnn.h"

namespace pnnx {

namespace ncnn {

class nn_RNN : public GraphRewriterPass
{
public:
    const char* match_pattern_graph() const
    {
        return R"PNNXIR(7767517
3 3
pnnx.Input              input       0 1 input
nn.RNN                  op_0        1 2 input out out_hidden input_size=%input_size hidden_size=%hidden_size num_layers=1 nonlinearity=%nonlinearity bias=%bias batch_first=%batch_first bidirectional=%bidirectional @weight_ih_l0 @weight_hh_l0 @bias_ih_l0 @bias_hh_l0 @weight_ih_l0_reverse @weight_hh_l0_reverse @bias_ih_l0_reverse @bias_hh_l0_reverse
pnnx.Output             output      2 0 out out_hidden
)PNNXIR";
    }

    const char* type_str() const
    {
        return "RNN";
    }

    const char* name_str() const
    {
        return "rnn";
    }

    void write(Operator* op, const std::map<std::string, Parameter>& captured_params, const std::map<std::string, Attribute>& captured_attrs) const
    {
        const std::string nonlinearity = captured_params.at("nonlinearity").s;

        if (nonlinearity != "tanh")
        {
            fprintf(stderr, "RNN nonlinearity=%s not supported\n", nonlinearity.c_str());
        }

        const bool bidirectional = captured_params.at("bidirectional").b;
        const int num_directions = bidirectional ? 2 : 1;
        const int num_output = captured_params.at("hidden_size").i;
        const int input_size = captured_params.at("input_size").i;

        int weight_data_size = num_directions * num_output * input_size;

        op->params["0"] = num_output;
        op->params["1"] = weight_data_size;
        op->params["2"] = bidirectional ? 2 : 0;

        op->attrs["0"] = Attribute();
        op->attrs["0"].data = {0, 0, 0, 0};
        if (bidirectional)
            op->attrs["1"] = captured_attrs.at("op_0.weight_ih_l0") + captured_attrs.at("op_0.weight_ih_l0_reverse");
        else
            op->attrs["1"] = captured_attrs.at("op_0.weight_ih_l0");

        op->attrs["2"] = Attribute();
        op->attrs["2"].data = {0, 0, 0, 0};
        if (captured_params.at("bias").b)
        {
            // reduce bias_ih and bias_hh
            std::vector<float> new_bias;
            {
                auto bias_ih = captured_attrs.at("op_0.bias_ih_l0").get_float32_data();
                auto bias_hh = captured_attrs.at("op_0.bias_hh_l0").get_float32_data();

                new_bias.resize(num_output);
                float* bias = (float*)new_bias.data();
                for (int i = 0; i < num_output; i++)
                {
                    bias[i] = bias_ih[i] + bias_hh[i];
                }
            }

            if (bidirectional)
            {
                std::vector<float> new_bias_reverse;
                {
                    auto bias_ih = captured_attrs.at("op_0.bias_ih_l0_reverse").get_float32_data();
                    auto bias_hh = captured_attrs.at("op_0.bias_hh_l0_reverse").get_float32_data();

                    new_bias_reverse.resize(num_output);
                    float* bias = (float*)new_bias_reverse.data();
                    for (int i = 0; i < num_output; i++)
                    {
                        bias[i] = bias_ih[i] + bias_hh[i];
                    }
                }

                op->attrs["3"] = Attribute({num_output}, new_bias) + Attribute({num_output}, new_bias_reverse);
            }
            else
            {
                op->attrs["3"] = Attribute({num_output}, new_bias);
            }
        }
        else
        {
            std::vector<float> bias(num_output, 0.f);

            if (bidirectional)
                op->attrs["3"] = Attribute({num_output}, bias) + Attribute({num_output}, bias);
            else
                op->attrs["3"] = Attribute({num_output}, bias);
        }

        op->attrs["4"] = Attribute();
        op->attrs["4"].data = {0, 0, 0, 0};
        if (bidirectional)
            op->attrs["5"] = captured_attrs.at("op_0.weight_hh_l0") + captured_attrs.at("op_0.weight_hh_l0_reverse");
        else
            op->attrs["5"] = captured_attrs.at("op_0.weight_hh_l0");
    }
};

REGISTER_GLOBAL_PNNX_NCNN_GRAPH_REWRITER_PASS(nn_RNN, 20)

class nn_RNN_1 : public nn_RNN
{
public:
    const char* match_pattern_graph() const
    {
        return R"PNNXIR(7767517
4 4
pnnx.Input              input       0 1 input
pnnx.Input              in_hidden   0 1 in_hidden
nn.RNN                  op_0        2 2 input in_hidden out out_hidden input_size=%input_size hidden_size=%hidden_size num_layers=1 nonlinearity=%nonlinearity bias=%bias batch_first=%batch_first bidirectional=%bidirectional @weight_ih_l0 @weight_hh_l0 @bias_ih_l0 @bias_hh_l0 @weight_ih_l0_reverse @weight_hh_l0_reverse @bias_ih_l0_reverse @bias_hh_l0_reverse
pnnx.Output             output      2 0 out out_hidden
)PNNXIR";
    }
};

REGISTER_GLOBAL_PNNX_NCNN_GRAPH_REWRITER_PASS(nn_RNN_1, 20)

class nn_RNN_2 : public nn_RNN
{
public:
    const char* match_pattern_graph() const
    {
        return R"PNNXIR(7767517
3 2
pnnx.Input              input       0 1 input
nn.RNN                  op_0        1 1 input out input_size=%input_size hidden_size=%hidden_size num_layers=1 nonlinearity=%nonlinearity bias=%bias batch_first=%batch_first bidirectional=%bidirectional @weight_ih_l0 @weight_hh_l0 @bias_ih_l0 @bias_hh_l0 @weight_ih_l0_reverse @weight_hh_l0_reverse @bias_ih_l0_reverse @bias_hh_l0_reverse
pnnx.Output             output      1 0 out
)PNNXIR";
    }
};

REGISTER_GLOBAL_PNNX_NCNN_GRAPH_REWRITER_PASS(nn_RNN_2, 20)

class nn_RNN_3 : public nn_RNN
{
public:
    const char* match_pattern_graph() const
    {
        return R"PNNXIR(7767517
4 3
pnnx.Input              input       0 1 input
pnnx.Input              in_hidden   0 1 in_hidden
nn.RNN                  op_0        2 1 input in_hidden out input_size=%input_size hidden_size=%hidden_size num_layers=1 nonlinearity=%nonlinearity bias=%bias batch_first=%batch_first bidirectional=%bidirectional @weight_ih_l0 @weight_hh_l0 @bias_ih_l0 @bias_hh_l0 @weight_ih_l0_reverse @weight_hh_l0_reverse @bias_ih_l0_reverse @bias_hh_l0_reverse
pnnx.Output             output      1 0 out
)PNNXIR";
    }
};

REGISTER_GLOBAL_PNNX_NCNN_GRAPH_REWRITER_PASS(nn_RNN_3, 20)

} // namespace ncnn

} // namespace pnnx
