#pragma once
#include "lowering_strategy.hpp"
#include "parser.hpp"

class lower_cabi : public lowering_strategy
{
public:
    explicit lower_cabi(context& ctx);

    void lower_function_arguments(const lir::function_head& func_head, arguments_type& args) override;
    void lower_function_argument(const type_kind& ty, const std::string& basename, arguments_type& args) override;
    void lower_local_variable(const symbol& sym, wasm_function& func) override;
    wasm_type lower_return_type(type_id return_type) override;

private:
    context& parse_context;
};