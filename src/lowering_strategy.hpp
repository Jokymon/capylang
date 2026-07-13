#pragma once
#include "lir.hpp"
#include "parser.hpp"
#include "wasm_ir.hpp"

class lowering_strategy
{
public:
    virtual ~lowering_strategy() = default;

    virtual void lower_function_arguments(const node_function_head& func_head, arguments_type& args) = 0;
    virtual void lower_function_argument(const type_kind& ty, const std::string& basename, arguments_type& args) = 0;
    virtual void lower_local_variables(const node_function_definition& func_def, wasm_function& func) = 0;
    virtual void lower_local_variable(const symbol& sym, wasm_function& func) = 0;

    virtual void lower_function_arguments(const lir::function_head& func_head, arguments_type& args) = 0;
    virtual wasm_type lower_return_type(type_id return_type) = 0;

    virtual void lower_variable_ref_read(symbol_id symbol_ref, wasm_block& output_block) = 0;
    virtual void lower_variable_ref_write(symbol_id symbol_ref, wasm_block& output_block) = 0;
};