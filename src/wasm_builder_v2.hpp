#pragma once

#include "wasm.hpp"

namespace wasm_builder_v2
{
    wasm::block& block(wasm::instruction_list& instructions, wasm::type_t return_type, const std::string& label);
    wasm::block& loop(wasm::instruction_list& instructions, wasm::type_t return_type, const std::string& label);
    wasm::if_block& if_block(wasm::instruction_list& instructions, wasm::type_t return_type, const std::string& label);

    void local_get(wasm::instruction_list& instructions, const std::string& name);
    void local_set(wasm::instruction_list& instructions, const std::string& name);
    void global_get(wasm::instruction_list& instructions, const std::string& name);
    void global_set(wasm::instruction_list& instructions, const std::string& name);
    void const_val(wasm::instruction_list& instructions, wasm::type_t type, int64_t value);
    void drop(wasm::instruction_list& instructions);
    void add(wasm::instruction_list& instructions, wasm::type_t type);
    void and_instr(wasm::instruction_list& instructions, wasm::type_t type);
    void div(wasm::instruction_list& instructions, wasm::type_t type);
    void mod(wasm::instruction_list& instructions, wasm::type_t type);
    void mul(wasm::instruction_list& instructions, wasm::type_t type);
    void or_instr(wasm::instruction_list& instructions, wasm::type_t type);
    void sub(wasm::instruction_list& instructions, wasm::type_t type);
    void shl(wasm::instruction_list& instructions, wasm::type_t type);
    void shr(wasm::instruction_list& instructions, wasm::type_t type);
    void load(wasm::instruction_list& instructions, wasm::type_t type, size_t offset = 0);
    void store(wasm::instruction_list& instructions, wasm::type_t type, size_t offset = 0);
    void eq(wasm::instruction_list& instructions, wasm::type_t type);
    void ne(wasm::instruction_list& instructions, wasm::type_t type);
    void eqz(wasm::instruction_list& instructions, wasm::type_t type);
    void lt(wasm::instruction_list& instructions, wasm::type_t type);
    void lte(wasm::instruction_list& instructions, wasm::type_t type);
    void gt(wasm::instruction_list& instructions, wasm::type_t type);
    void gte(wasm::instruction_list& instructions, wasm::type_t type);
    void br(wasm::instruction_list& instructions, const wasm::branch_label& label);
    void br_if(wasm::instruction_list& instructions, const wasm::branch_label& label);
    void call(wasm::instruction_list& instructions, const std::string& function_name);
    void ret(wasm::instruction_list& instructions);
    void memory_size(wasm::instruction_list& instructions);
    void memory_grow(wasm::instruction_list& instructions);
    void memory_fill(wasm::instruction_list& instructions);
    void i64_extend_i32_s(wasm::instruction_list& instructions);
    void i64_extend_i32_u(wasm::instruction_list& instructions);
    void i32_wrap_i64(wasm::instruction_list& instructions);
    void unreachable(wasm::instruction_list& instructions);
}
