#pragma once
#include "ir.hpp"
#include <ostream>
#include <string>
#include <vector>

class wasm_generator
{
public:
    void generate(const wasm_module& module, std::ostream &output);

private:
    void generate_types(const wasm_module& module, std::ostream &output);
    void generate_imports(const wasm_module& module, std::ostream &output);
    void generate_functions(const wasm_module& module, std::ostream &output);
    void generate_memories(const wasm_module& module, std::ostream &output);
    void generate_globals(const wasm_module& module, std::ostream &output);
    void generate_exports(const wasm_module& module, std::ostream &output);
    void generate_code(const wasm_module& module, std::ostream &output);

    void generate_block(const wasm_module& module, const wasm_function& function, const wasm_block& block, std::ostream& output);

private:
    size_t intern_func_type(const wasm_function& function);

private:
    std::vector<std::string> type_entries;
};