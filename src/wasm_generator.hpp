#pragma once
#include "wasm.hpp"
#include <map>
#include <ostream>
#include <string>
#include <vector>

class wasm_generator_v2
{
public:
    wasm_generator_v2();
    void generate(const wasm::module& module, std::ostream& output);

private:
    void generate_types(const wasm::module& module, std::ostream& output);
    void generate_imports(const wasm::module& module, std::ostream& output);
    void generate_functions(const wasm::module& module, std::ostream& output);
    void generate_memories(const wasm::module& module, std::ostream& output);
    void generate_globals(const wasm::module& module, std::ostream& output);
    void generate_exports(const wasm::module& module, std::ostream& output);
    void generate_code(const wasm::module& module, std::ostream& output);
    void generate_data(const wasm::module& module, std::ostream& output);
    void generate_names(const wasm::module& module, std::ostream& output);

    void generate_expr(
        const wasm::module& module,
        const wasm::function* function,
        const wasm::instruction_list& expr,
        std::ostream& output
    );
    void generate_block(
        const wasm::module& module,
        const wasm::function* function,
        const wasm::block& block,
        std::ostream& output
    );

    size_t intern_func_type(const wasm::function& function);
    void index_module(const wasm::module& module);
    size_t local_index(const wasm::function& function, const std::string& name) const;
    uint32_t resolve_block_label(const wasm::branch_label& label) const;

private:
    std::vector<std::string> type_entries;
    std::map<std::string, size_t> function_indices;
    std::map<std::string, size_t> global_indices;
    std::vector<std::string> control_labels;
};
