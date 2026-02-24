#pragma once
#include "wasm_ir.hpp"
#include <ostream>

class wat_generator
{
public:
    void generate(const wasm_module& module, std::ostream &output);

private:
    void generate(const wasm_function& function, std::ostream &output, size_t indent);
    void generate(const wasm_data_section& section, std::ostream &output, size_t indent);
    void generate(const wasm_memory& memory, std::ostream &output, size_t indent);
    void generate(const wasm_block& block, std::ostream &output, size_t indent);
    void generate(const wasm_if_block& block, std::ostream &output, size_t indent);
    void generate(const wasm_loop_block& block, std::ostream &output, size_t indent);
    void generate(const wasm_internal_block& block, std::ostream &output, size_t indent);

    void generate(const wasm_instruction&, std::ostream &output, size_t indent);
    void generate(const wasm_op_index&, std::ostream &output, size_t indent);
    void generate(const wasm_op_type&, std::ostream &output, size_t indent);
    void generate(const wasm_op_type_sign&, std::ostream &output, size_t indent);
    void generate(const wasm_op_type_value&, std::ostream &output, size_t indent);
    void generate(const wasm_op_align_offset&, std::ostream &output, size_t indent);
    void generate(const wasm_op_label&, std::ostream &output, size_t indent);
    void generate(const wasm_op_func&, std::ostream &output, size_t indent);

    void generate_export(const exportable& exp, std::ostream &output, size_t indent);
};