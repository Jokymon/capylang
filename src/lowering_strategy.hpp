#pragma once
#include "parser.hpp"
#include "wasm_ir.hpp"

class lowering_strategy
{
public:
    virtual ~lowering_strategy() = default;

    virtual void lower_function_arguments(const node_function_head& func_head, arguments_type& args) = 0;

    virtual void lower_variable_ref_read(symbol_id symbol_ref, wasm_block& output_block) = 0;
    virtual void lower_variable_ref_write(symbol_id symbol_ref, wasm_block& output_block) = 0;
};