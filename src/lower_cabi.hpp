#pragma once
#include "lowering_strategy.hpp"
#include "parser.hpp"

class lower_cabi : public lowering_strategy
{
public:
    explicit lower_cabi(context& ctx);

    void lower_function_arguments(const node_function_head& func_head, arguments_type& args) override;

private:
    context& parse_context;
};