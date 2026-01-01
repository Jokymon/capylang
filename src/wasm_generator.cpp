#include "wasm_generator.hpp"
#include "wasm_mnemonics.hpp"
#include <algorithm>
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
        if (value) b |= 0x80;
        out.put(b);
    } while (value!=0);
}

void encode_string(std::ostream& out, const std::string s)
{
    encode_leb128(out, s.size());
    out << s;
}

char encode_wasm_type(wasm_type type)
{
    switch (type) {
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

void wasm_generator::generate(const wasm_module& module, std::ostream &output)
{
    output.write(MAGIC, 4);
    output.write(VERSION, 4);

    generate_types(module, output);
    generate_imports(module, output);
    generate_functions(module, output);
    generate_memories(module, output);
    generate_globals(module, output);
    generate_exports(module, output);
}

void wasm_generator::generate_types(const wasm_module& module, std::ostream &output)
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

void wasm_generator::generate_imports(const wasm_module& module, std::ostream &output)
{
    output.put(SECTION_IMPORT);

    size_t import_count = std::count_if(module.functions.begin(), module.functions.end(),
        [](const auto& func) {
            return func.is_imported();
        });

    std::ostringstream content(std::ios::binary);
    encode_leb128(content, import_count);

    for (const auto& func : module.functions)
    {
        if (!func.is_imported())
            continue;

        encode_string(content, func.ns);
        encode_string(content, func.name);

        content.put(EXTERNAL_TYPE_FUNC);
        size_t index = intern_func_type(func);
        encode_leb128(content, index);
    }

    encode_leb128(output, content.str().size());
    output.write(content.str().c_str(), content.str().size());
}

void wasm_generator::generate_functions(const wasm_module& module, std::ostream &output)
{
    output.put(SECTION_FUNCTION);

    size_t internal_count = std::count_if(module.functions.begin(), module.functions.end(),
        [](const auto& func_pair) {
            return !func_pair.is_imported();
        });

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

void wasm_generator::generate_memories(const wasm_module& module, std::ostream &output)
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

void wasm_generator::generate_globals(const wasm_module& module, std::ostream &output)
{
    output.put(SECTION_GLOBAL);

    std::ostringstream content(std::ios::binary);
    encode_leb128(content, module.globals.size());
    for (const auto& g : module.globals)
    {
        content.put(encode_wasm_type(g.typ));
        if (g.access==wasm_module::access_type::mut)
            content.put(MUTABILITY_MUT);
        else
            content.put(MUTABILITY_IMMUT);

        content.put(INST_I32_CONST);
        encode_leb128(content, g.initvalue);
        content.put(INST_TERMINATOR);
    }

    encode_leb128(output, content.str().size());
    output.write(content.str().c_str(), content.str().size());
}

void wasm_generator::generate_exports(const wasm_module& module, std::ostream &output)
{
    output.put(SECTION_EXPORT);

    std::ostringstream content(std::ios::binary);
    encode_leb128(content, module.exports.size());
    for (const auto& exp : module.exports)
    {
        encode_string(content, exp.get().export_name);
        content.put(encode_extern_index(exp.get().export_type));
        encode_leb128(content, exp.get().index);
    }

    encode_leb128(output, content.str().size());
    output.write(content.str().c_str(), content.str().size());
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
