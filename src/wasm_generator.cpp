#include "wasm_generator.hpp"
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

void encode_func_type(std::ostream& output, const wasm_function& function)
{
    output.put(COMP_TYPE_FUNC);
    output.put((char)function.arguments.size());
    for (const auto& arg : function.arguments)
    {
        output.put(encode_wasm_type(arg.second));
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
        if (!func.second.is_imported())
            continue;

        intern_func_type(func.second);
    }
    for (const auto& func : module.functions)
    {
        if (func.second.is_imported())
            continue;

        intern_func_type(func.second);
    }

    std::stringstream content;
    content.put((char)type_entries.size());
    for (const auto& entry : type_entries)
    {
        content.write(entry.c_str(), entry.size());
    }

    output.put((char)content.str().size());
    output.write(content.str().c_str(), content.str().size());
}

void wasm_generator::generate_imports(const wasm_module& module, std::ostream &output)
{
    output.put(SECTION_IMPORT);

    size_t import_count = std::count_if(module.functions.begin(), module.functions.end(),
        [](const auto& func_pair) {
            return func_pair.second.is_imported();
        });

    std::stringstream content;
    content.put((char)import_count);

    for (const auto& func : module.functions)
    {
        if (!func.second.is_imported())
            continue;

        content.put((char)func.second.ns.size());
        content << func.second.ns;
        content.put((char)func.second.name.size());
        content << func.second.name;

        content.put(EXTERNAL_TYPE_FUNC);
        size_t index = intern_func_type(func.second);
        content.put((char)index);
    }

    output.put((char)content.str().size());
    output.write(content.str().c_str(), content.str().size());
}

void wasm_generator::generate_functions(const wasm_module& module, std::ostream &output)
{
    output.put(SECTION_FUNCTION);

    size_t internal_count = std::count_if(module.functions.begin(), module.functions.end(),
        [](const auto& func_pair) {
            return !func_pair.second.is_imported();
        });

    std::stringstream content;
    content.put((char)internal_count);

    for (const auto& func : module.functions)
    {
        if (func.second.is_imported())
            continue;

        size_t index = intern_func_type(func.second);
        content.put((char)index);
    }

    output.put((char)content.str().size());
    output.write(content.str().c_str(), content.str().size());
}

size_t wasm_generator::intern_func_type(const wasm_function& function)
{
    std::stringstream func_type_entry;
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
