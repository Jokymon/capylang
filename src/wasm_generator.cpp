#include "wasm_generator.hpp"
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
}

void wasm_generator::generate_types(const wasm_module& module, std::ostream &output)
{
    output.put(SECTION_TYPE);

    std::vector<std::string> type_entries;
    for (const auto& func : module.functions)
    {
        // First generate for the imported function types
        // TODO: This is just at the moment to keep the WAT and
        // WASM generated parts identical for easier comparison,
        // maybe later we could also drop this
        if (!func.second.is_imported())
            continue;

        std::stringstream func_type_entry;
        encode_func_type(func_type_entry, func.second);

        if (std::find(type_entries.begin(), type_entries.end(), func_type_entry.str()) == type_entries.end())
        {
            type_entries.push_back(func_type_entry.str());
        }
    }
    for (const auto& func : module.functions)
    {
        if (func.second.is_imported())
            continue;

        std::stringstream func_type_entry;
        encode_func_type(func_type_entry, func.second);

        if (std::find(type_entries.begin(), type_entries.end(), func_type_entry.str()) == type_entries.end())
        {
            type_entries.push_back(func_type_entry.str());
        }
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
