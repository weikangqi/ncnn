// Copyright 2023 Tencent
// SPDX-License-Identifier: BSD-3-Clause

#include "fuse_layernorm.h"

#include "pass_level2.h"

#include <math.h>
#include <string.h>

namespace pnnx {

class fuse_layernorm_pass : public GraphRewriterPass
{
public:
    const char* match_pattern_graph() const
    {
        return R"PNNXIR(7767517
8 7
pnnx.Input              input       0 1 input #input=(1,%c,?,?)f32
pnnx.Attribute          op_0        0 1 weight @data #weight=(%c,1,1)f32
pnnx.Attribute          op_1        0 1 bias @data #bias=(%c,1,1)f32
torch.mean              op_2        1 1 input mean dim=(1) keepdim=True
pnnx.Expression         op_3        2 1 input mean 2 expr=pow(sub(@0,@1),2)
torch.mean              op_4        1 1 2 var dim=(1) keepdim=True
pnnx.Expression         op_5        5 1 weight input mean var bias out expr=add(mul(@0,div(sub(@1,@2),sqrt(add(@3,%eps)))),@4)
pnnx.Output             output      1 0 out
)PNNXIR";
    }

    const char* replace_pattern_graph() const
    {
        return R"PNNXIR(7767517
5 4
pnnx.Input              input       0 1 input
Tensor.permute          op_0        1 1 input a dims=(0,2,3,1)
nn.LayerNorm            op_1        1 1 a b elementwise_affine=True eps=%eps normalized_shape=(%c) @weight=%op_0.data @bias=%op_1.data
Tensor.permute          op_2        1 1 b out dims=(0,3,1,2)
pnnx.Output             output      1 0 out
)PNNXIR";
    }

    void write(const std::map<std::string, Operator*>& ops, const std::map<std::string, Parameter>& captured_params, const std::map<std::string, Attribute>& captured_attrs) const
    {
        GraphRewriterPass::write(ops, captured_params, captured_attrs);

        // fix weight bias shape from (c,1,1) to (c)
        const int c = captured_params.at("c").i;
        Operator* op_1 = ops.at("op_1");
        op_1->attrs["weight"].shape = {c};
        op_1->attrs["bias"].shape = {c};
    }
};

void fuse_layernorm(Graph& graph)
{
    fuse_layernorm_pass a;
    int opindex = 0;

    pnnx_graph_rewrite(graph, &a, opindex);
}

} // namespace pnnx
