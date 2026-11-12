#include "wasm_generator_v2.hpp"

#include "tools.hpp"
#include "wasm_mnemonics.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <sstream>
#include <type_traits>
#include <variant>

namespace
{
    const char* MAGIC = "\0asm";
    const char* VERSION = "\x01\0\0\0";

    const char SECTION_CUSTOM = '\x00';
    const char SECTION_TYPE = '\x01';
    const char SECTION_IMPORT = '\x02';
    const char SECTION_FUNCTION = '\x03';
    const char SECTION_MEMORY = '\x05';
    const char SECTION_GLOBAL = '\x06';
    const char SECTION_EXPORT = '\x07';
    const char SECTION_CODE = '\x0A';
    const char SECTION_DATA = '\x0B';

    const char NAME_SUBSECTION_FUNCTION_NAMES = '\x01';
    const char NAME_SUBSECTION_LOCAL_NAMES = '\x02';
    const char NAME_SUBSECTION_GLOBAL_NAMES = '\x07';

    const char COMP_TYPE_FUNC = '\x60';

    const char EXTERNAL_TYPE_FUNC = '\x00';
    const char EXTERNAL_TYPE_TABLE = '\x01';
    const char EXTERNAL_TYPE_MEM = '\x02';
    const char EXTERNAL_TYPE_GLOBAL = '\x03';
    const char EXTERNAL_TYPE_TAG = '\x04';

    const char LIMITS_I32_N = '\x00';
    const char LIMITS_I32_NM = '\x01';

    const char MUTABILITY_IMMUT = '\x00';
    const char MUTABILITY_MUT = '\x01';

    void encode_leb128(std::ostream& output, uint64_t value)
    {
        do
        {
            uint8_t byte = value & 0x7f;
            value >>= 7;
            if (value != 0) byte |= 0x80;
            output.put(byte);
        } while (value != 0);
    }

    void encode_leb128_signed(std::ostream& output, int64_t value)
    {
        bool more = true;
        while (more)
        {
            uint8_t byte = value & 0x7f;
            value >>= 7;

            uint8_t sign_bit = byte & 0x40;
            if ((value == 0 && sign_bit == 0) || (value == -1 && sign_bit != 0)) more = false;
            else byte |= 0x80;

            output.put(byte);
        }
    }

    void encode_i32_const_immediate(std::ostream& output, int64_t value)
    {
        encode_leb128_signed(output, static_cast<int32_t>(static_cast<uint32_t>(value)));
    }

    void encode_string(std::ostream& output, const std::string& value)
    {
        encode_leb128(output, value.size());
        output.write(value.data(), value.size());
    }

    void write_section(std::ostream& output, char section, const std::ostringstream& content)
    {
        const auto data = content.str();
        output.put(section);
        encode_leb128(output, data.size());
        output.write(data.data(), data.size());
    }

    char encode_wasm_type(wasm::type_t type)
    {
        switch (type)
        {
            case wasm::type_t::U8:
            case wasm::type_t::U16:
            case wasm::type_t::U32:
            case wasm::type_t::I8:
            case wasm::type_t::I16:
            case wasm::type_t::I32:
                return '\x7F';
            case wasm::type_t::U64:
            case wasm::type_t::I64:
                return '\x7E';
            case wasm::type_t::F32:
                return '\x7D';
            case wasm::type_t::F64:
                return '\x7C';
            case wasm::type_t::None:
                CAPY_FAIL("Trying to encode void as a WASM value type");
                return '\x40';
        }
    }

    char encode_extern_index(wasm::extern_index_t index)
    {
        switch (index)
        {
            case wasm::extern_index_t::FuncIdx: return EXTERNAL_TYPE_FUNC;
            case wasm::extern_index_t::TableIdx: return EXTERNAL_TYPE_TABLE;
            case wasm::extern_index_t::MemIdx: return EXTERNAL_TYPE_MEM;
            case wasm::extern_index_t::GlobalIdx: return EXTERNAL_TYPE_GLOBAL;
            case wasm::extern_index_t::TagIdx: return EXTERNAL_TYPE_TAG;
        }
    }

    void encode_func_type(std::ostream& output, const wasm::function& function)
    {
        output.put(COMP_TYPE_FUNC);
        encode_leb128(output, function.arguments.size());
        for (const auto& argument : function.arguments)
        {
            output.put(encode_wasm_type(argument.type));
        }

        if (function.return_type == wasm::type_t::None)
        {
            output.put(0);
        }
        else
        {
            output.put(1);
            output.put(encode_wasm_type(function.return_type));
        }
    }

    bool is_imported(const wasm::importable& object)
    {
        return !object.ns.empty();
    }

    bool is_wasm_type_signed(wasm::type_t type)
    {
        switch (type)
        {
            case wasm::type_t::I8:
            case wasm::type_t::I16:
            case wasm::type_t::I32:
            case wasm::type_t::I64:
            case wasm::type_t::F32:
            case wasm::type_t::F64:
                return true;
            case wasm::type_t::None:
            case wasm::type_t::U8:
            case wasm::type_t::U16:
            case wasm::type_t::U32:
            case wasm::type_t::U64:
                return false;
        }
    }

    bool is_i32_stack_type(wasm::type_t type)
    {
        switch (type)
        {
            case wasm::type_t::U8:
            case wasm::type_t::U16:
            case wasm::type_t::U32:
            case wasm::type_t::I8:
            case wasm::type_t::I16:
            case wasm::type_t::I32:
                return true;
            default:
                return false;
        }
    }

    bool is_i64_stack_type(wasm::type_t type)
    {
        return type == wasm::type_t::I64 || type == wasm::type_t::U64;
    }

    bool is_integer_stack_type(wasm::type_t type)
    {
        return is_i32_stack_type(type) || is_i64_stack_type(type);
    }

    char select_instruction(wasm::type_t type, char i32_instruction, char i64_instruction)
    {
        CAPY_ASSERT(is_integer_stack_type(type), "Expected an integer WASM value type");
        return is_i64_stack_type(type) ? i64_instruction : i32_instruction;
    }

    char select_signed_instruction(
        wasm::type_t type,
        char i32_signed,
        char i32_unsigned,
        char i64_signed,
        char i64_unsigned
    )
    {
        CAPY_ASSERT(is_integer_stack_type(type), "Expected an integer WASM value type");
        if (is_i64_stack_type(type))
            return is_wasm_type_signed(type) ? i64_signed : i64_unsigned;
        return is_wasm_type_signed(type) ? i32_signed : i32_unsigned;
    }

    enum class wasm_size
    {
        Size8,
        Size16,
        Size32,
        Size64
    };

    wasm_size wasm_type_size(wasm::type_t type)
    {
        switch (type)
        {
            case wasm::type_t::I8:
            case wasm::type_t::U8:
                return wasm_size::Size8;
            case wasm::type_t::I16:
            case wasm::type_t::U16:
                return wasm_size::Size16;
            case wasm::type_t::None:
            case wasm::type_t::U32:
            case wasm::type_t::I32:
            case wasm::type_t::F32:
                return wasm_size::Size32;
            case wasm::type_t::U64:
            case wasm::type_t::I64:
            case wasm::type_t::F64:
                return wasm_size::Size64;
        }
    }

    void encode_limits(std::ostream& output, const wasm::memory& memory)
    {
        if (memory.max_block_count > 0)
        {
            output.put(LIMITS_I32_NM);
            encode_leb128(output, memory.initial_block_count);
            encode_leb128(output, memory.max_block_count);
        }
        else
        {
            output.put(LIMITS_I32_N);
            encode_leb128(output, memory.initial_block_count);
        }
    }
}

wasm_generator_v2::wasm_generator_v2()
: type_entries()
, function_indices()
, global_indices()
, control_labels()
{
}

void wasm_generator_v2::generate(const wasm::module& module, std::ostream& output)
{
    type_entries.clear();
    control_labels.clear();
    index_module(module);

    output.write(MAGIC, 4);
    output.write(VERSION, 4);

    generate_types(module, output);
    generate_imports(module, output);
    generate_functions(module, output);
    generate_memories(module, output);
    generate_globals(module, output);
    generate_exports(module, output);
    generate_code(module, output);
    generate_data(module, output);
    generate_names(module, output);
}

void wasm_generator_v2::generate_types(const wasm::module& module, std::ostream& output)
{
    for (const auto& function : module.functions)
    {
        if (is_imported(*function)) intern_func_type(*function);
    }
    for (const auto& function : module.functions)
    {
        if (!is_imported(*function)) intern_func_type(*function);
    }

    std::ostringstream content(std::ios::binary);
    encode_leb128(content, type_entries.size());
    for (const auto& entry : type_entries) content.write(entry.data(), entry.size());
    write_section(output, SECTION_TYPE, content);
}

void wasm_generator_v2::generate_imports(const wasm::module& module, std::ostream& output)
{
    size_t import_count = std::count_if(
        module.memories.begin(), module.memories.end(),
        [](const auto& memory) { return is_imported(*memory); }
    );
    import_count += std::count_if(
        module.functions.begin(), module.functions.end(),
        [](const auto& function) { return is_imported(*function); }
    );

    std::ostringstream content(std::ios::binary);
    encode_leb128(content, import_count);

    for (const auto& memory : module.memories)
    {
        if (!is_imported(*memory)) continue;
        encode_string(content, memory->ns);
        encode_string(content, memory->import_name);
        content.put(EXTERNAL_TYPE_MEM);
        encode_limits(content, *memory);
    }

    for (const auto& function : module.functions)
    {
        if (!is_imported(*function)) continue;
        encode_string(content, function->ns);
        encode_string(content, function->import_name);
        content.put(EXTERNAL_TYPE_FUNC);
        encode_leb128(content, intern_func_type(*function));
    }

    write_section(output, SECTION_IMPORT, content);
}

void wasm_generator_v2::generate_functions(const wasm::module& module, std::ostream& output)
{
    size_t internal_count = std::count_if(
        module.functions.begin(), module.functions.end(),
        [](const auto& function) { return !is_imported(*function); }
    );

    std::ostringstream content(std::ios::binary);
    encode_leb128(content, internal_count);
    for (const auto& function : module.functions)
    {
        if (is_imported(*function)) continue;
        encode_leb128(content, intern_func_type(*function));
    }
    write_section(output, SECTION_FUNCTION, content);
}

void wasm_generator_v2::generate_memories(const wasm::module& module, std::ostream& output)
{
    size_t internal_count = std::count_if(
        module.memories.begin(), module.memories.end(),
        [](const auto& memory) { return !is_imported(*memory); }
    );

    std::ostringstream content(std::ios::binary);
    encode_leb128(content, internal_count);
    for (const auto& memory : module.memories)
    {
        if (is_imported(*memory)) continue;
        encode_limits(content, *memory);
    }
    write_section(output, SECTION_MEMORY, content);
}

void wasm_generator_v2::generate_globals(const wasm::module& module, std::ostream& output)
{
    std::ostringstream content(std::ios::binary);
    encode_leb128(content, module.globals.size());
    for (const auto& global : module.globals)
    {
        content.put(encode_wasm_type(global->typ));
        content.put(global->access == wasm::access_t::Mut ? MUTABILITY_MUT : MUTABILITY_IMMUT);
        generate_expr(module, nullptr, global->init_expression, content);
        content.put(INST_TERMINATOR);
    }
    write_section(output, SECTION_GLOBAL, content);
}

void wasm_generator_v2::generate_exports(const wasm::module& module, std::ostream& output)
{
    std::ostringstream content(std::ios::binary);
    encode_leb128(content, module.exports.size());
    for (const auto& export_item : module.exports)
    {
        encode_string(content, export_item->export_name);
        content.put(encode_extern_index(export_item->export_type));
        encode_leb128(content, export_item->index);
    }
    write_section(output, SECTION_EXPORT, content);
}

void wasm_generator_v2::generate_code(const wasm::module& module, std::ostream& output)
{
    size_t internal_count = std::count_if(
        module.functions.begin(), module.functions.end(),
        [](const auto& function) { return !is_imported(*function); }
    );

    std::ostringstream content(std::ios::binary);
    encode_leb128(content, internal_count);

    for (const auto& function : module.functions)
    {
        if (is_imported(*function)) continue;

        std::ostringstream function_code(std::ios::binary);
        size_t local_group_count = 0;
        char previous_type = 0;
        for (const auto& local : function->locals)
        {
            char current_type = encode_wasm_type(local.type);
            if (local_group_count == 0 || current_type != previous_type)
            {
                local_group_count++;
                previous_type = current_type;
            }
        }

        encode_leb128(function_code, local_group_count);
        size_t local_index = 0;
        while (local_index < function->locals.size())
        {
            char type = encode_wasm_type(function->locals[local_index].type);
            size_t group_end = local_index + 1;
            while (group_end < function->locals.size() &&
                   encode_wasm_type(function->locals[group_end].type) == type)
            {
                group_end++;
            }

            encode_leb128(function_code, group_end - local_index);
            function_code.put(type);
            local_index = group_end;
        }

        generate_expr(module, function.get(), function->function_body, function_code);
        function_code.put(INST_TERMINATOR);

        const auto function_data = function_code.str();
        encode_leb128(content, function_data.size());
        content.write(function_data.data(), function_data.size());
    }

    write_section(output, SECTION_CODE, content);
}

void wasm_generator_v2::generate_data(const wasm::module& module, std::ostream& output)
{
    std::ostringstream content(std::ios::binary);
    encode_leb128(content, module.data_sections.size());
    for (const auto& data : module.data_sections)
    {
        encode_leb128(content, 0);
        content.put(INST_I32_CONST);
        encode_i32_const_immediate(content, data->init_offset);
        content.put(INST_TERMINATOR);
        encode_leb128(content, data->data_buffer.size());
        content.write(data->data_buffer.data(), data->data_buffer.size());
    }
    write_section(output, SECTION_DATA, content);
}

void wasm_generator_v2::generate_names(const wasm::module& module, std::ostream& output)
{
    std::ostringstream content(std::ios::binary);
    encode_string(content, "name");

    std::ostringstream function_names(std::ios::binary);
    encode_leb128(function_names, module.functions.size());
    auto emit_function_names = [&](bool imported)
    {
        for (const auto& function : module.functions)
        {
            if (is_imported(*function) != imported) continue;
            encode_leb128(function_names, function_indices.at(function->name));
            encode_string(function_names, function->name);
        }
    };
    emit_function_names(true);
    emit_function_names(false);

    content.put(NAME_SUBSECTION_FUNCTION_NAMES);
    const auto function_names_data = function_names.str();
    encode_leb128(content, function_names_data.size());
    content.write(function_names_data.data(), function_names_data.size());

    std::ostringstream local_names(std::ios::binary);
    encode_leb128(local_names, module.functions.size());
    auto emit_local_names = [&](bool imported)
    {
        for (const auto& function : module.functions)
        {
            if (is_imported(*function) != imported) continue;
            encode_leb128(local_names, function_indices.at(function->name));
            encode_leb128(local_names, function->arguments.size() + function->locals.size());

            size_t index = 0;
            for (const auto& argument : function->arguments)
            {
                encode_leb128(local_names, index++);
                encode_string(local_names, argument.name);
            }
            for (const auto& local : function->locals)
            {
                encode_leb128(local_names, index++);
                encode_string(local_names, local.name);
            }
        }
    };
    emit_local_names(true);
    emit_local_names(false);

    content.put(NAME_SUBSECTION_LOCAL_NAMES);
    const auto local_names_data = local_names.str();
    encode_leb128(content, local_names_data.size());
    content.write(local_names_data.data(), local_names_data.size());

    std::ostringstream global_names(std::ios::binary);
    encode_leb128(global_names, module.globals.size());
    for (const auto& global : module.globals)
    {
        encode_leb128(global_names, global_indices.at(global->name));
        encode_string(global_names, global->name);
    }

    content.put(NAME_SUBSECTION_GLOBAL_NAMES);
    const auto global_names_data = global_names.str();
    encode_leb128(content, global_names_data.size());
    content.write(global_names_data.data(), global_names_data.size());
    write_section(output, SECTION_CUSTOM, content);
}

void wasm_generator_v2::generate_expr(
    const wasm::module& module,
    const wasm::function* function,
    const wasm::instruction_list& expr,
    std::ostream& output
)
{
    for (const auto& instruction : expr)
    {
        CAPY_ASSERT(instruction != nullptr, "Encountered null instruction during WASM generation");
        std::visit(
            [&](const auto& value)
            {
                using T = std::decay_t<decltype(value)>;

                if constexpr (std::is_same_v<T, wasm::op_no_arg>)
                {
                    switch (value.op)
                    {
                        case wasm::operation_t::Nop:
                            output.put(INST_NOP);
                            break;
                        case wasm::operation_t::Unreachable:
                            output.put(INST_UNREACHABLE);
                            break;
                        case wasm::operation_t::Drop:
                            output.put(INST_DROP);
                            break;
                        case wasm::operation_t::Ret:
                            output.put(INST_RET);
                            break;
                        case wasm::operation_t::MemorySize:
                            output.put(INST_MEMORY_SIZE);
                            encode_leb128(output, 0);
                            break;
                        case wasm::operation_t::MemoryGrow:
                            output.put(INST_MEMORY_GROW);
                            encode_leb128(output, 0);
                            break;
                        case wasm::operation_t::MemoryFill:
                            output.put(INST_FC);
                            encode_leb128(output, INST_FC_MEMORY_FILL);
                            encode_leb128(output, 0);
                            break;
                        case wasm::operation_t::I64ExtendI32S:
                            output.put(INST_I64_EXTEND_I32_S);
                            break;
                        case wasm::operation_t::I64ExtendI32U:
                            output.put(INST_I64_EXTEND_I32_U);
                            break;
                        case wasm::operation_t::I32WrapI64:
                            output.put(INST_I32_WRAP_I64);
                            break;
                        default:
                            CAPY_FAIL("Unexpected no-argument operation in WASM generator");
                            break;
                    }
                }
                else if constexpr (std::is_same_v<T, wasm::op_index>)
                {
                    if (value.op == wasm::operation_t::Call)
                    {
                        output.put(INST_CALL);
                        encode_leb128(output, function_indices.at(value.name));
                        return;
                    }

                    size_t index = 0;
                    switch (value.op)
                    {
                        case wasm::operation_t::LocalGet:
                            CAPY_ASSERT(function != nullptr, "Local access is not valid in a global initializer");
                            output.put(INST_LOCAL_GET);
                            index = local_index(*function, value.name);
                            break;
                        case wasm::operation_t::LocalSet:
                            CAPY_ASSERT(function != nullptr, "Local access is not valid in a global initializer");
                            output.put(INST_LOCAL_SET);
                            index = local_index(*function, value.name);
                            break;
                        case wasm::operation_t::LocalTee:
                            CAPY_ASSERT(function != nullptr, "Local access is not valid in a global initializer");
                            output.put(INST_LOCAL_TEE);
                            index = local_index(*function, value.name);
                            break;
                        case wasm::operation_t::GlobalGet:
                            output.put(INST_GLOBAL_GET);
                            index = global_indices.at(value.name);
                            break;
                        case wasm::operation_t::GlobalSet:
                            output.put(INST_GLOBAL_SET);
                            index = global_indices.at(value.name);
                            break;
                        default:
                            CAPY_FAIL("Unexpected indexed operation in WASM generator");
                            break;
                    }
                    encode_leb128(output, index);
                }
                else if constexpr (std::is_same_v<T, wasm::op_type>)
                {
                    switch (value.op)
                    {
                        case wasm::operation_t::IAdd: output.put(select_instruction(value.value_type, INST_I32_ADD, INST_I64_ADD)); break;
                        case wasm::operation_t::ISub: output.put(select_instruction(value.value_type, INST_I32_SUB, INST_I64_SUB)); break;
                        case wasm::operation_t::IMul: output.put(select_instruction(value.value_type, INST_I32_MUL, INST_I64_MUL)); break;
                        case wasm::operation_t::IAnd: output.put(select_instruction(value.value_type, INST_I32_AND, INST_I64_AND)); break;
                        case wasm::operation_t::IOr: output.put(select_instruction(value.value_type, INST_I32_OR, INST_I64_OR)); break;
                        case wasm::operation_t::IShl: output.put(select_instruction(value.value_type, INST_I32_SHL, INST_I64_SHL)); break;
                        case wasm::operation_t::Eq: output.put(select_instruction(value.value_type, INST_I32_EQ, INST_I64_EQ)); break;
                        case wasm::operation_t::Ne: output.put(select_instruction(value.value_type, INST_I32_NE, INST_I64_NE)); break;
                        case wasm::operation_t::Eqz: output.put(select_instruction(value.value_type, INST_I32_EQZ, INST_I64_EQZ)); break;
                        default:
                            CAPY_FAIL("Unexpected typed operation in WASM generator");
                            break;
                    }
                }
                else if constexpr (std::is_same_v<T, wasm::op_type_sign>)
                {
                    switch (value.op)
                    {
                        case wasm::operation_t::IDiv: output.put(select_signed_instruction(value.value_type, INST_I32_DIV_S, INST_I32_DIV_U, INST_I64_DIV_S, INST_I64_DIV_U)); break;
                        case wasm::operation_t::IRem: output.put(select_signed_instruction(value.value_type, INST_I32_REM_S, INST_I32_REM_U, INST_I64_REM_S, INST_I64_REM_U)); break;
                        case wasm::operation_t::IShr: output.put(select_signed_instruction(value.value_type, INST_I32_SHR_S, INST_I32_SHR_U, INST_I64_SHR_S, INST_I64_SHR_U)); break;
                        case wasm::operation_t::ILt: output.put(select_signed_instruction(value.value_type, INST_I32_LT_S, INST_I32_LT_U, INST_I64_LT_S, INST_I64_LT_U)); break;
                        case wasm::operation_t::ILte: output.put(select_signed_instruction(value.value_type, INST_I32_LE_S, INST_I32_LE_U, INST_I64_LE_S, INST_I64_LE_U)); break;
                        case wasm::operation_t::IGt: output.put(select_signed_instruction(value.value_type, INST_I32_GT_S, INST_I32_GT_U, INST_I64_GT_S, INST_I64_GT_U)); break;
                        case wasm::operation_t::IGte: output.put(select_signed_instruction(value.value_type, INST_I32_GE_S, INST_I32_GE_U, INST_I64_GE_S, INST_I64_GE_U)); break;
                        default:
                            CAPY_FAIL("Unexpected signed typed operation in WASM generator");
                            break;
                    }
                }
                else if constexpr (std::is_same_v<T, wasm::op_type_value>)
                {
                    CAPY_ASSERT(value.op == wasm::operation_t::Const, "Unexpected typed-value operation in WASM generator");
                    switch (wasm_type_size(value.value_type))
                    {
                        case wasm_size::Size8:
                        case wasm_size::Size16:
                        case wasm_size::Size32:
                            CAPY_ASSERT(value.value_type != wasm::type_t::F32, "Floating-point constants are not represented by the current IR");
                            output.put(INST_I32_CONST);
                            encode_i32_const_immediate(output, value.value);
                            break;
                        case wasm_size::Size64:
                            CAPY_ASSERT(value.value_type != wasm::type_t::F64, "Floating-point constants are not represented by the current IR");
                            output.put(INST_I64_CONST);
                            encode_leb128_signed(output, value.value);
                            break;
                    }
                }
                else if constexpr (std::is_same_v<T, wasm::op_align_offset>)
                {
                    auto size = wasm_type_size(value.value_type);
                    CAPY_ASSERT(
                        value.value_type != wasm::type_t::F32 &&
                        value.value_type != wasm::type_t::F64 &&
                        value.value_type != wasm::type_t::None,
                        "Only integer memory operations are currently supported"
                    );

                    if (value.op == wasm::operation_t::Load)
                    {
                        switch (size)
                        {
                            case wasm_size::Size8:
                                output.put(is_wasm_type_signed(value.value_type) ? INST_I32_LOAD8_S : INST_I32_LOAD8_U);
                                break;
                            case wasm_size::Size16:
                                output.put(is_wasm_type_signed(value.value_type) ? INST_I32_LOAD16_S : INST_I32_LOAD16_U);
                                break;
                            case wasm_size::Size32:
                                output.put(INST_I32_LOAD);
                                break;
                            case wasm_size::Size64:
                                output.put(INST_I64_LOAD);
                                break;
                        }
                    }
                    else if (value.op == wasm::operation_t::Store)
                    {
                        switch (size)
                        {
                            case wasm_size::Size8: output.put(INST_I32_STORE8); break;
                            case wasm_size::Size16: output.put(INST_I32_STORE16); break;
                            case wasm_size::Size32: output.put(INST_I32_STORE); break;
                            case wasm_size::Size64: output.put(INST_I64_STORE); break;
                        }
                    }
                    else
                    {
                        CAPY_FAIL("Unexpected memory operation in WASM generator");
                    }

                    encode_leb128(output, value.alignment);
                    encode_leb128(output, value.offset);
                }
                else if constexpr (std::is_same_v<T, wasm::op_label>)
                {
                    if (value.op == wasm::operation_t::Br) output.put(INST_BR);
                    else if (value.op == wasm::operation_t::BrIf) output.put(INST_BR_IF);
                    else
                    {
                        CAPY_FAIL("Unexpected label operation in WASM generator");
                    }
                    encode_leb128(output, resolve_block_label(value.label));
                }
                else if constexpr (std::is_same_v<T, wasm::if_block>)
                {
                    output.put(INST_IF);
                    output.put(value.return_type == wasm::type_t::None ? '\x40' : encode_wasm_type(value.return_type));

                    CAPY_ASSERT(value.block_label != nullptr, "If block is missing a branch label");
                    control_labels.push_back(value.block_label->name);
                    generate_expr(module, function, value.then_block, output);
                    if (!value.else_block.empty())
                    {
                        output.put(INST_ELSE);
                        generate_expr(module, function, value.else_block, output);
                    }
                    control_labels.pop_back();
                    output.put(INST_TERMINATOR);
                }
                else if constexpr (std::is_same_v<T, wasm::block>)
                {
                    generate_block(module, function, value, output);
                }
            },
            *instruction
        );
    }
}

void wasm_generator_v2::generate_block(
    const wasm::module& module,
    const wasm::function* function,
    const wasm::block& block,
    std::ostream& output
)
{
    output.put(block.block_type == wasm::block_type_t::Block ? INST_BLOCK : INST_LOOP);
    output.put(block.return_type == wasm::type_t::None ? '\x40' : encode_wasm_type(block.return_type));

    CAPY_ASSERT(block.block_label != nullptr, "Block is missing a branch label");
    control_labels.push_back(block.block_label->name);
    generate_expr(module, function, block.instructions, output);
    control_labels.pop_back();
    output.put(INST_TERMINATOR);
}

size_t wasm_generator_v2::intern_func_type(const wasm::function& function)
{
    std::ostringstream entry(std::ios::binary);
    encode_func_type(entry, function);

    auto existing = std::find(type_entries.begin(), type_entries.end(), entry.str());
    if (existing != type_entries.end()) return std::distance(type_entries.begin(), existing);

    type_entries.push_back(entry.str());
    return type_entries.size() - 1;
}

void wasm_generator_v2::index_module(const wasm::module& module)
{
    function_indices.clear();
    global_indices.clear();

    size_t function_index = 0;
    for (const auto& function : module.functions)
    {
        if (is_imported(*function)) function_indices[function->name] = function_index++;
    }
    for (const auto& function : module.functions)
    {
        if (!is_imported(*function)) function_indices[function->name] = function_index++;
    }

    for (size_t index = 0; index < module.globals.size(); index++)
    {
        global_indices[module.globals[index]->name] = index;
    }
}

size_t wasm_generator_v2::local_index(const wasm::function& function, const std::string& name) const
{
    for (size_t index = 0; index < function.arguments.size(); index++)
    {
        if (function.arguments[index].name == name) return index;
    }
    for (size_t index = 0; index < function.locals.size(); index++)
    {
        if (function.locals[index].name == name) return function.arguments.size() + index;
    }

    CAPY_FAIL("Unknown local '%s' in function '%s'", name.c_str(), function.name.c_str());
    return 0;
}

uint32_t wasm_generator_v2::resolve_block_label(const wasm::branch_label& label) const
{
    for (size_t depth = 0; depth < control_labels.size(); depth++)
    {
        size_t index = control_labels.size() - depth - 1;
        if (control_labels[index] == label.name) return static_cast<uint32_t>(depth);
    }

    CAPY_FAIL("Can't resolve block label '%s' in jump instruction", label.name.c_str());
    return std::numeric_limits<uint32_t>::max();
}
