#include "wasm_generator.hpp"
#include "wasm_mnemonics.hpp"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <variant>

const char* MAGIC = "\0asm";
const char* VERSION = "\x01\0\0\0";

const char SECTION_CUSTOM = '\x00';
const char SECTION_TYPE = '\x01';
const char SECTION_IMPORT = '\x02';
const char SECTION_FUNCTION = '\x03';
const char SECTION_TABLE = '\x04';
const char SECTION_MEMORY = '\x05';
const char SECTION_GLOBAL = '\x06';
const char SECTION_EXPORT = '\x07';
const char SECTION_START = '\x08';
const char SECTION_ELEMENT = '\x09';
const char SECTION_CODE = '\x0A';
const char SECTION_DATA = '\x0B';
const char SECTION_DATA_COUNT = '\x0C';
const char SECTION_TAG = '\x0D';

const char NAME_SUBSECTION_MODULE_NAME = '\x00';
const char NAME_SUBSECTION_FUNCTION_NAMES = '\x01';
const char NAME_SUBSECTION_LOCAL_NAMES = '\x02';
const char NAME_SUBSECTION_TYPE_NAMES = '\x04';
const char NAME_SUBSECTION_GLOBAL_NAMES = '\x07'; // extended name section proposal
const char NAME_SUBSECTION_FIELD_NAMES = '\x0a';
const char NAME_SUBSECTION_TAG_NAMES = '\x0b';

const char COMP_TYPE_FUNC = '\x60';

const char EXTERNAL_TYPE_FUNC = '\x00';
const char EXTERNAL_TYPE_TABLE = '\x01';
const char EXTERNAL_TYPE_MEM = '\x02';
const char EXTERNAL_TYPE_GLOBAL = '\x03';
const char EXTERNAL_TYPE_TAG = '\x04';

const char LIMITS_I32_N = '\x00';
const char LIMITS_I32_NM = '\x01';
const char LIMITS_I64_N = '\x04';
const char LIMITS_I64_NM = '\x05';

const char MUTABILITY_IMMUT = '\x00';
const char MUTABILITY_MUT = '\x01';

void encode_leb128(std::ostream& out, uint64_t value)
{
    do
    {
        uint8_t b = value & 0x7f;
        value >>= 7;
        if (value)
            b |= 0x80;
        out.put(b);
    } while (value != 0);
}

void encode_leb128_signed(std::ostream& out, int64_t value)
{
    bool more = true;

    while (more)
    {
        uint8_t b = value & 0x7f; /* low-order 7 bits of value */
        value >>= 7;

        /* sign bit of byte is second high-order bit (0x40) */
        uint8_t sign_bit = b & 0x40;
        if ((value == 0 && sign_bit == 0) || (value == -1 && sign_bit != 0))
            more = false;
        else
            b |= 0x80; /* set high-order bit of byte */

        out.put(b);
    }
}

int64_t normalize_i32_immediate(int64_t value)
{
    // WASM i32.const uses signed LEB128, but the payload represents 32-bit bits.
    // Normalize through uint32_t first so values like 4294967295 become -1.
    return static_cast<int32_t>(static_cast<uint32_t>(value));
}

void encode_i32_const_immediate(std::ostream& out, int64_t value)
{
    encode_leb128_signed(out, normalize_i32_immediate(value));
}

void encode_string(std::ostream& out, const std::string s)
{
    encode_leb128(out, s.size());
    out << s;
}

char encode_wasm_type(wasm_type type)
{
    switch (type)
    {
        case wasm_type::i32:
        case wasm_type::u32:
            return '\x7F';
        case wasm_type::i64:
        case wasm_type::u64:
            return '\x7E';
        case wasm_type::f32:
            return '\x7D';
        case wasm_type::f64:
            return '\x7C';
        default:
            assert(false && "Trying to encode an unknown WASM type");
            // TODO shouldn't happen
            return '\x42';
    }
}

char encode_extern_index(wasm_extern_index index)
{
    switch (index)
    {
        case wasm_extern_index::funcidx:
            return EXTERNAL_TYPE_FUNC;
        case wasm_extern_index::tableidx:
            return EXTERNAL_TYPE_TABLE;
        case wasm_extern_index::memidx:
            return EXTERNAL_TYPE_MEM;
        case wasm_extern_index::globalidx:
            return EXTERNAL_TYPE_GLOBAL;
        case wasm_extern_index::tagidx:
            return EXTERNAL_TYPE_TAG;
    }
}

void encode_func_type(std::ostream& output, const wasm_function& function)
{
    output.put(COMP_TYPE_FUNC);
    encode_leb128(output, function.arguments.size());
    for (const auto& arg : function.arguments)
    {
        output.put(encode_wasm_type(arg.type));
    }

    if (function.return_type != wasm_type::none)
    {
        output.put(1);
        output.put(encode_wasm_type(function.return_type));
    }
    else
    {
        output.put(0);
    }
}

bool is_wasm_type_signed(wasm_type typ)
{
    switch (typ)
    {
        case wasm_type::none:
            return false;
        case wasm_type::i8:
        case wasm_type::i16:
        case wasm_type::i32:
        case wasm_type::i64:
        case wasm_type::f32:
        case wasm_type::f64:
            return true;
        case wasm_type::u8:
        case wasm_type::u16:
        case wasm_type::u32:
        case wasm_type::u64:
            return false;
    }
}

enum wasm_size_e
{
    Size8,
    Size16,
    Size32,
    Size64
};

wasm_size_e wasm_type_size(wasm_type typ)
{
    switch (typ)
    {
        case wasm_type::none:
        case wasm_type::u32:
        case wasm_type::i32:
        case wasm_type::f32:
            return wasm_size_e::Size32;
        case wasm_type::u64:
        case wasm_type::i64:
        case wasm_type::f64:
            return wasm_size_e::Size64;
        case wasm_type::i8:
        case wasm_type::u8:
            return wasm_size_e::Size8;
        case wasm_type::i16:
        case wasm_type::u16:
            return wasm_size_e::Size16;
    }
}

uint32_t resolve_block_label(const wasm_block& block, const wasm_branch_label& label, uint32_t nesting_level)
{
    if (label == block.label())
    {
        return nesting_level;
    }
    if (block.enclosing_block == nullptr)
    {
        return std::numeric_limits<uint32_t>::max();
    }
    return resolve_block_label(*block.enclosing_block, label, nesting_level + 1);
}

void wasm_generator::generate(const wasm_module& module, std::ostream& output)
{
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

void wasm_generator::generate_types(const wasm_module& module, std::ostream& output)
{
    output.put(SECTION_TYPE);

    for (const auto& func : module.functions)
    {
        // First generate for the imported function types
        // TODO: This is just at the moment to keep the WAT and
        // WASM generated parts identical for easier comparison,
        // maybe later we could also drop this
        if (!func.is_imported())
            continue;

        intern_func_type(func);
    }
    for (const auto& func : module.functions)
    {
        if (func.is_imported())
            continue;

        intern_func_type(func);
    }

    std::ostringstream content(std::ios::binary);
    encode_leb128(content, type_entries.size());
    for (const auto& entry : type_entries)
    {
        content.write(entry.c_str(), entry.size());
    }

    encode_leb128(output, content.str().size());
    output.write(content.str().c_str(), content.str().size());
}

void wasm_generator::generate_imports(const wasm_module& module, std::ostream& output)
{
    output.put(SECTION_IMPORT);

    size_t import_count = std::count_if(
        module.functions.begin(),
        module.functions.end(),
        [](const auto& func)
        { return func.is_imported(); }
    );

    std::ostringstream content(std::ios::binary);
    encode_leb128(content, import_count);

    for (const auto& func : module.functions)
    {
        if (!func.is_imported())
            continue;

        encode_string(content, func.ns);
        encode_string(content, func.import_name);

        content.put(EXTERNAL_TYPE_FUNC);
        size_t index = intern_func_type(func);
        encode_leb128(content, index);
    }

    encode_leb128(output, content.str().size());
    output.write(content.str().c_str(), content.str().size());
}

void wasm_generator::generate_functions(const wasm_module& module, std::ostream& output)
{
    output.put(SECTION_FUNCTION);

    size_t internal_count = std::count_if(module.functions.begin(), module.functions.end(), [](const auto& func_pair)
                                          { return !func_pair.is_imported(); });

    std::ostringstream content(std::ios::binary);
    encode_leb128(content, internal_count);

    for (const auto& func : module.functions)
    {
        if (func.is_imported())
            continue;

        size_t index = intern_func_type(func);
        encode_leb128(content, index);
    }

    encode_leb128(output, content.str().size());
    output.write(content.str().c_str(), content.str().size());
}

void wasm_generator::generate_memories(const wasm_module& module, std::ostream& output)
{
    output.put(SECTION_MEMORY);

    std::ostringstream content(std::ios::binary);
    encode_leb128(content, module.memories.size());
    for (const auto& mem : module.memories)
    {
        content.put(LIMITS_I32_N);
        encode_leb128(content, mem.initial_block_count);
    }

    encode_leb128(output, content.str().size());
    output.write(content.str().c_str(), content.str().size());
}

void wasm_generator::generate_globals(const wasm_module& module, std::ostream& output)
{
    output.put(SECTION_GLOBAL);

    std::ostringstream content(std::ios::binary);
    encode_leb128(content, module.globals.size());
    for (const auto& g : module.globals)
    {
        content.put(encode_wasm_type(g.typ));
        if (g.access == wasm_module::access_type::mut)
            content.put(MUTABILITY_MUT);
        else
            content.put(MUTABILITY_IMMUT);

        content.put(INST_I32_CONST);
        encode_i32_const_immediate(content, static_cast<int64_t>(g.initvalue));
        content.put(INST_TERMINATOR);
    }

    encode_leb128(output, content.str().size());
    output.write(content.str().c_str(), content.str().size());
}

void wasm_generator::generate_exports(const wasm_module& module, std::ostream& output)
{
    output.put(SECTION_EXPORT);

    std::ostringstream content(std::ios::binary);
    encode_leb128(content, module.exports.size());
    for (const auto& exp : module.exports)
    {
        encode_string(content, exp.export_name);
        content.put(encode_extern_index(exp.export_type));
        encode_leb128(content, exp.index);
    }

    encode_leb128(output, content.str().size());
    output.write(content.str().c_str(), content.str().size());
}

void wasm_generator::generate_code(const wasm_module& module, std::ostream& output)
{
    output.put(SECTION_CODE);

    size_t internal_count = std::count_if(module.functions.begin(), module.functions.end(), [](const auto& func_pair)
                                          { return !func_pair.is_imported(); });

    std::ostringstream content(std::ios::binary);
    encode_leb128(content, internal_count);

    for (const auto& func : module.functions)
    {
        if (func.is_imported())
            continue;

        std::ostringstream function_code(std::ios::binary);

        // Let's start with a simple assumption of only
        // having one 'locals' entry of only i32
        encode_leb128(function_code, 1);
        encode_leb128(function_code, func.locals.size());
        function_code.put(encode_wasm_type(wasm_type::i32));

        generate_block(module, func, func.body_const(), function_code);
        function_code.put(INST_TERMINATOR);

        encode_leb128(content, function_code.str().size());
        content.write(function_code.str().c_str(), function_code.str().size());
    }

    encode_leb128(output, content.str().size());
    output.write(content.str().c_str(), content.str().size());
}

void wasm_generator::generate_data(const wasm_module& module, std::ostream& output)
{
    output.put(SECTION_DATA);

    std::ostringstream content(std::ios::binary);
    encode_leb128(content, module.data_sections.size());
    for (const auto& data : module.data_sections)
    {
        encode_leb128(content, 0); // active data section in memory 0

        content.put(INST_I32_CONST);
        encode_leb128_signed(content, data.init_offset);
        content.put(INST_TERMINATOR);

        encode_leb128(content, data.data_buffer.size());
        content << data.data_buffer;
    }

    encode_leb128(output, content.str().size());
    output.write(content.str().c_str(), content.str().size());
}

void wasm_generator::generate_names(const wasm_module& module, std::ostream& output)
{
    output.put(SECTION_CUSTOM);

    std::ostringstream content(std::ios::binary);
    encode_string(content, "name");

    // Add function names: name map
    std::ostringstream subsection1(std::ios::binary);
    encode_leb128(subsection1, module.functions.size());
    for (size_t index = 0; index < module.functions.size(); ++index)
    {
        encode_leb128(subsection1, index);
        encode_string(subsection1, module.functions[index].name);
    }

    content.put(NAME_SUBSECTION_FUNCTION_NAMES);
    encode_leb128(content, subsection1.str().size());
    content.write(subsection1.str().c_str(), subsection1.str().size());

    // Add parameter/local names: indirect name map
    std::ostringstream subsection2(std::ios::binary);
    encode_leb128(subsection2, module.functions.size());
    for (size_t index = 0; index < module.functions.size(); ++index)
    {
        encode_leb128(subsection2, index);

        const wasm_function& func{module.functions[index]};
        // number of arguments and locals of this function
        encode_leb128(subsection2, func.arguments.size() + func.locals.size());
        size_t local_index = 0;
        for (const auto& param : func.arguments)
        {
            encode_leb128(subsection2, local_index++);
            encode_string(subsection2, param.name);
        }
        for (const auto& param : func.locals)
        {
            encode_leb128(subsection2, local_index++);
            encode_string(subsection2, param.name);
        }
    }

    content.put(NAME_SUBSECTION_LOCAL_NAMES);
    encode_leb128(content, subsection2.str().size());
    content.write(subsection2.str().c_str(), subsection2.str().size());

    // Add global names
    std::ostringstream subsection7(std::ios::binary);
    encode_leb128(subsection7, module.globals.size());
    for (size_t index = 0; index < module.globals.size(); ++index)
    {
        encode_leb128(subsection7, index);
        encode_string(subsection7, module.globals[index].name);
    }

    content.put(NAME_SUBSECTION_GLOBAL_NAMES);
    encode_leb128(content, subsection7.str().size());
    content.write(subsection7.str().c_str(), subsection7.str().size());

    encode_leb128(output, content.str().size());
    output.write(content.str().c_str(), content.str().size());
}

void wasm_generator::generate_block(const wasm_module& module, const wasm_function& function, const wasm_block& block, std::ostream& output)
{
    switch (block.block_type)
    {
        case wasm_block::t_plain:
            break;
        case wasm_block::t_block:
            output.put(INST_BLOCK);
            output.put(0x40);
            break;
        case wasm_block::t_loop:
            output.put(INST_LOOP);
            output.put(0x40);
            break;
    }

    for (const auto& inst : block.instructions)
    {
        std::visit([&](const auto& t)
                   {
                       using T = std::decay_t<decltype(t)>;

                       if constexpr (std::is_same_v<T, wasm_instruction>)
                       {
                           switch (t.op)
                           {
                               case wasm_op::drop:
                                   output.put(INST_DROP);
                                   break;
                               case wasm_op::ret:
                                   output.put(INST_RET);
                                   break;
                               case wasm_op::memory_size:
                                   output.put(INST_MEMORY_SIZE);
                                   encode_leb128(output, 0);  // memory index is hardcoded for the moment
                                   break;
                               case wasm_op::memory_grow:
                                   output.put(INST_MEMORY_GROW);
                                   encode_leb128(output, 0);  // memory index is hardcoded for the moment
                                   break;
                               default:
                                   break;
                           }
                       }
                       else if constexpr (std::is_same_v<T, wasm_op_type>)
                       {
                           switch (t.op)
                           {
                               // TODO: differentiate based on type
                               case wasm_op::iadd:
                                   output.put(INST_I32_ADD);
                                   break;
                               case wasm_op::isub:
                                   output.put(INST_I32_SUB);
                                   break;
                               case wasm_op::imul:
                                   output.put(INST_I32_MUL);
                                   break;
                               case wasm_op::iand:
                                   output.put(INST_I32_AND);
                                   break;
                               case wasm_op::ior:
                                   output.put(INST_I32_OR);
                                   break;
                               case wasm_op::ishl:
                                   output.put(INST_I32_SHL);
                                   break;
                               case wasm_op::eq:
                                   output.put(INST_I32_EQ);
                                   break;
                               case wasm_op::ne:
                                   output.put(INST_I32_NE);
                                   break;
                               case wasm_op::eqz:
                                   output.put(INST_I32_EQZ);
                                   break;
                               default:
                                   break;
                           }
                       }
                       else if constexpr (std::is_same_v<T, wasm_op_index>)
                       {
                           size_t index = 0;
                           switch (t.op)
                           {
                               case wasm_op::local_get:
                                   output.put(INST_LOCAL_GET);
                                   break;
                               case wasm_op::local_set:
                                   output.put(INST_LOCAL_SET);
                                   break;
                               case wasm_op::local_tee:
                                   output.put(INST_LOCAL_TEE);
                                   break;
                               case wasm_op::global_get:
                                   output.put(INST_GLOBAL_GET);
                                   break;
                               case wasm_op::global_set:
                                   output.put(INST_GLOBAL_SET);
                                   break;
                               default:
                                   break;
                           }
                           switch (t.op)
                           {
                               case wasm_op::local_get:
                               case wasm_op::local_set:
                               case wasm_op::local_tee:
                                   index = function.locals_map.at(t.name);
                                   break;
                               case wasm_op::global_get:
                               case wasm_op::global_set:
                                   index = module.globals_map.at(t.name);
                                   break;
                               default:
                                   break;
                           }
                           encode_leb128(output, index);
                       }
                       else if constexpr (std::is_same_v<T, wasm_op_type_sign>)
                       {
                           switch (t.op)
                           {
                               case wasm_op::idiv:
                                   if (is_wasm_type_signed(t.value_type))
                                       output.put(INST_I32_DIV_S);
                                   else
                                       output.put(INST_I32_DIV_U);
                                   break;
                               case wasm_op::irem:
                                   if (is_wasm_type_signed(t.value_type))
                                       output.put(INST_I32_REM_S);
                                   else
                                       output.put(INST_I32_REM_U);
                                   break;
                               case wasm_op::ishr:
                                   if (is_wasm_type_signed(t.value_type))
                                       output.put(INST_I32_SHR_S);
                                   else
                                       output.put(INST_I32_SHR_U);
                                   break;
                               case wasm_op::ilt:
                                   if (is_wasm_type_signed(t.value_type))
                                       output.put(INST_I32_LT_S);
                                   else
                                       output.put(INST_I32_LT_U);
                                   break;
                               case wasm_op::ilte:
                                   if (is_wasm_type_signed(t.value_type))
                                       output.put(INST_I32_LE_S);
                                   else
                                       output.put(INST_I32_LE_U);
                                   break;
                               case wasm_op::igt:
                                   if (is_wasm_type_signed(t.value_type))
                                       output.put(INST_I32_GT_S);
                                   else
                                       output.put(INST_I32_GT_U);
                                   break;
                               case wasm_op::igte:
                                   if (is_wasm_type_signed(t.value_type))
                                       output.put(INST_I32_GE_S);
                                   else
                                       output.put(INST_I32_GE_U);
                                   break;
                               default:
                                   break;
                           }
                       }
                       else if constexpr (std::is_same_v<T, wasm_op_type_value>)
                       {
                           // TODO: differentiate by type of the operation
                           output.put(INST_I32_CONST);
                           encode_i32_const_immediate(output, t.value);
                       }
                       else if constexpr (std::is_same_v<T, wasm_op_align_offset>)
                       {

                           switch (t.op)
                           {
                               case wasm_op::load:
                                   switch (wasm_type_size(t.value_type))
                                   {
                                       case wasm_size_e::Size8:
                                           if (is_wasm_type_signed(t.value_type))
                                           {
                                               output.put(INST_I32_LOAD8_S);
                                           }
                                           else
                                           {
                                               output.put(INST_I32_LOAD8_U);
                                           }
                                           break;
                                       default:
                                           output.put(INST_I32_LOAD);
                                           break;
                                   }
                                   encode_leb128(output, t.alignment);
                                   encode_leb128(output, t.offset);
                                   break;
                               case wasm_op::store:
                                   switch (wasm_type_size(t.value_type))
                                   {
                                       case wasm_size_e::Size8:
                                           output.put(INST_I32_STORE8);
                                           break;
                                       default:
                                           output.put(INST_I32_STORE);
                                           break;
                                   }
                                   encode_leb128(output, t.alignment);
                                   encode_leb128(output, t.offset);
                                   break;
                               default:
                                   break;
                           }
                       }
                       else if constexpr (std::is_same_v<T, wasm_op_label>)
                       {
                           uint32_t block_label = resolve_block_label(block, t.label, 0);
                           assert(block_label != std::numeric_limits<uint32_t>::max() || "Can't resolve block label in jump instruction");

                           switch (t.op)
                           {
                               case wasm_op::br:
                                   output.put(INST_BR);
                                   break;
                               case wasm_op::br_if:
                                   output.put(INST_BR_IF);
                                   break;
                               default:
                                   break;
                           }

                           encode_leb128(output, block_label);
                       }
                       else if constexpr (std::is_same_v<T, wasm_op_func>)
                       {
                           output.put(INST_CALL);
                           size_t function_index = module.functions_map.at(t.function.name());
                           encode_leb128(output, function_index);
                       }
                       else if constexpr (std::is_same_v<T, wasm_if_block>)
                       {
                           output.put(INST_IF);
                           if (t.return_type != wasm_type::none)
                           {
                               output.put(encode_wasm_type(t.return_type));
                           }
                           else
                           {
                               output.put(0x40); // block type 'void'
                           }

                           generate_block(module, function, *(t.then_block), output);

                           if (t.else_block)
                           {
                               output.put(INST_ELSE);
                               generate_block(module, function, *(t.else_block), output);
                           }

                           output.put(INST_TERMINATOR);
                       }
                       else if constexpr (std::is_same_v<T, wasm_block>)
                       {
                        generate_block(module, function, t, output);
                       } },
                   *inst);
    }

    switch (block.block_type)
    {
        case wasm_block::t_plain:
            break;
        case wasm_block::t_block:
        case wasm_block::t_loop:
            output.put(INST_TERMINATOR);
            break;
    }
}

size_t wasm_generator::intern_func_type(const wasm_function& function)
{
    std::ostringstream func_type_entry(std::ios::binary);
    encode_func_type(func_type_entry, function);

    auto existing_entry = std::find(type_entries.begin(), type_entries.end(), func_type_entry.str());
    if (existing_entry == type_entries.end())
    {
        type_entries.push_back(func_type_entry.str());
        return type_entries.size() - 1;
    }
    else
    {
        return std::distance(type_entries.begin(), existing_entry);
    }
}
