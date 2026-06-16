#pragma once
#include "lowering_strategy.hpp"
#include "parser.hpp"

class lower_cabi : public lowering_strategy
{
public:
    explicit lower_cabi(context& ctx);

    void lower_function_arguments(const node_function_head& func_head, arguments_type& args) override;
    void lower_function_argument(const type_kind& ty, const std::string& basename, arguments_type& args) override;
    void lower_local_variables(const node_function_definition& func_def, wasm_function& func) override;
    void lower_local_variable(const symbol& sym, wasm_function& func) override;

    void lower_variable_ref_read(symbol_id symbol_ref, wasm_block& output_block) override;
    void lower_variable_ref_write(symbol_id symbol_ref, wasm_block& output_block) override;

private:
    context& parse_context;
};