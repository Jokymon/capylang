#include "wasm_builder.hpp"

namespace
{
    template <typename T>
    T& append(wasm::instruction_list& instructions, T instruction)
    {
        auto node = std::make_unique<wasm::instruction>(std::move(instruction));
        auto& result = std::get<T>(*node);
        instructions.push_back(std::move(node));
        return result;
    }

    void append_no_arg(wasm::instruction_list& instructions, wasm::operation_t operation)
    {
        append(instructions, wasm::op_no_arg{operation});
    }

    void append_index(wasm::instruction_list& instructions, wasm::operation_t operation, const std::string& name)
    {
        append(instructions, wasm::op_index{operation, name});
    }

    void append_type(wasm::instruction_list& instructions, wasm::operation_t operation, wasm::type_t type)
    {
        append(instructions, wasm::op_type{operation, type});
    }

    void append_signed_type(wasm::instruction_list& instructions, wasm::operation_t operation, wasm::type_t type)
    {
        append(instructions, wasm::op_type_sign{operation, type});
    }
}

namespace wasm_builder
{
    wasm::block& block(wasm::instruction_list& instructions, wasm::type_t return_type, const std::string& label)
    {
        return append(instructions, wasm::block{{return_type, std::make_unique<wasm::branch_label>(wasm::branch_label{label})}, wasm::block_type_t::Block, {}});
    }

    wasm::block& loop(wasm::instruction_list& instructions, wasm::type_t return_type, const std::string& label)
    {
        return append(instructions, wasm::block{{return_type, std::make_unique<wasm::branch_label>(wasm::branch_label{label})}, wasm::block_type_t::Loop, {}});
    }

    wasm::if_block& if_block(wasm::instruction_list& instructions, wasm::type_t return_type, const std::string& label)
    {
        return append(instructions, wasm::if_block{{return_type, std::make_unique<wasm::branch_label>(wasm::branch_label{label})}, {}, {}});
    }

    void local_get(wasm::instruction_list& instructions, const std::string& name)
    {
        append_index(instructions, wasm::operation_t::LocalGet, name);
    }
    void local_set(wasm::instruction_list& instructions, const std::string& name)
    {
        append_index(instructions, wasm::operation_t::LocalSet, name);
    }
    void global_get(wasm::instruction_list& instructions, const std::string& name)
    {
        append_index(instructions, wasm::operation_t::GlobalGet, name);
    }
    void global_set(wasm::instruction_list& instructions, const std::string& name)
    {
        append_index(instructions, wasm::operation_t::GlobalSet, name);
    }

    void const_val(wasm::instruction_list& instructions, wasm::type_t type, int64_t value)
    {
        append(instructions, wasm::op_type_value{wasm::operation_t::Const, type, value});
    }

    void drop(wasm::instruction_list& instructions)
    {
        append_no_arg(instructions, wasm::operation_t::Drop);
    }
    void add(wasm::instruction_list& instructions, wasm::type_t type)
    {
        append_type(instructions, wasm::operation_t::IAdd, type);
    }
    void and_instr(wasm::instruction_list& instructions, wasm::type_t type)
    {
        append_type(instructions, wasm::operation_t::IAnd, type);
    }
    void div(wasm::instruction_list& instructions, wasm::type_t type)
    {
        append_signed_type(instructions, wasm::operation_t::IDiv, type);
    }
    void mod(wasm::instruction_list& instructions, wasm::type_t type)
    {
        append_signed_type(instructions, wasm::operation_t::IRem, type);
    }
    void mul(wasm::instruction_list& instructions, wasm::type_t type)
    {
        append_type(instructions, wasm::operation_t::IMul, type);
    }
    void or_instr(wasm::instruction_list& instructions, wasm::type_t type)
    {
        append_type(instructions, wasm::operation_t::IOr, type);
    }
    void sub(wasm::instruction_list& instructions, wasm::type_t type)
    {
        append_type(instructions, wasm::operation_t::ISub, type);
    }
    void shl(wasm::instruction_list& instructions, wasm::type_t type)
    {
        append_type(instructions, wasm::operation_t::IShl, type);
    }
    void shr(wasm::instruction_list& instructions, wasm::type_t type)
    {
        append_signed_type(instructions, wasm::operation_t::IShr, type);
    }

    void load(wasm::instruction_list& instructions, wasm::type_t type, size_t offset)
    {
        append(instructions, wasm::op_align_offset{wasm::operation_t::Load, type, 0, offset});
    }

    void store(wasm::instruction_list& instructions, wasm::type_t type, size_t offset)
    {
        append(instructions, wasm::op_align_offset{wasm::operation_t::Store, type, 0, offset});
    }

    void eq(wasm::instruction_list& instructions, wasm::type_t type)
    {
        append_type(instructions, wasm::operation_t::Eq, type);
    }
    void ne(wasm::instruction_list& instructions, wasm::type_t type)
    {
        append_type(instructions, wasm::operation_t::Ne, type);
    }
    void eqz(wasm::instruction_list& instructions, wasm::type_t type)
    {
        append_type(instructions, wasm::operation_t::Eqz, type);
    }
    void lt(wasm::instruction_list& instructions, wasm::type_t type)
    {
        append_signed_type(instructions, wasm::operation_t::ILt, type);
    }
    void lte(wasm::instruction_list& instructions, wasm::type_t type)
    {
        append_signed_type(instructions, wasm::operation_t::ILte, type);
    }
    void gt(wasm::instruction_list& instructions, wasm::type_t type)
    {
        append_signed_type(instructions, wasm::operation_t::IGt, type);
    }
    void gte(wasm::instruction_list& instructions, wasm::type_t type)
    {
        append_signed_type(instructions, wasm::operation_t::IGte, type);
    }

    void br(wasm::instruction_list& instructions, const wasm::branch_label& label)
    {
        append(instructions, wasm::op_label{wasm::operation_t::Br, label});
    }
    void br_if(wasm::instruction_list& instructions, const wasm::branch_label& label)
    {
        append(instructions, wasm::op_label{wasm::operation_t::BrIf, label});
    }
    void call(wasm::instruction_list& instructions, const std::string& function_name)
    {
        append_index(instructions, wasm::operation_t::Call, function_name);
    }
    void ret(wasm::instruction_list& instructions)
    {
        append_no_arg(instructions, wasm::operation_t::Ret);
    }
    void memory_size(wasm::instruction_list& instructions)
    {
        append_no_arg(instructions, wasm::operation_t::MemorySize);
    }
    void memory_grow(wasm::instruction_list& instructions)
    {
        append_no_arg(instructions, wasm::operation_t::MemoryGrow);
    }
    void memory_fill(wasm::instruction_list& instructions)
    {
        append_no_arg(instructions, wasm::operation_t::MemoryFill);
    }
    void i64_extend_i32_s(wasm::instruction_list& instructions)
    {
        append_no_arg(instructions, wasm::operation_t::I64ExtendI32S);
    }
    void i64_extend_i32_u(wasm::instruction_list& instructions)
    {
        append_no_arg(instructions, wasm::operation_t::I64ExtendI32U);
    }
    void i32_wrap_i64(wasm::instruction_list& instructions)
    {
        append_no_arg(instructions, wasm::operation_t::I32WrapI64);
    }
    void unreachable(wasm::instruction_list& instructions)
    {
        append_no_arg(instructions, wasm::operation_t::Unreachable);
    }
}
