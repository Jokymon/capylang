#pragma once
#include "wasm.hpp"
#include <ostream>

class wat_generator_v2
{
public:
    void generate(const wasm::module& module, std::ostream& output);

private:
    void generate(const wasm::function& function, std::ostream& output, size_t indent);
    void generate(const wasm::data_section& section, std::ostream& output, size_t indent);
    void generate(const wasm::memory& memory, std::ostream& output, size_t indent);
    void generate(const wasm::global& global, std::ostream& output, size_t indent);
    void generate(const wasm::block& block, std::ostream& output, size_t indent);
    void generate(const wasm::if_block& block, std::ostream& output, size_t indent);

    void generate(const wasm::instruction_list& expr, std::ostream& output, size_t indent);
    void generate(const wasm::op_no_arg& instruction, std::ostream& output, size_t indent);
    void generate(const wasm::op_index& instruction, std::ostream& output, size_t indent);
    void generate(const wasm::op_type& instruction, std::ostream& output, size_t indent);
    void generate(const wasm::op_type_sign& instruction, std::ostream& output, size_t indent);
    void generate(const wasm::op_type_value& instruction, std::ostream& output, size_t indent);
    void generate(const wasm::op_align_offset& instruction, std::ostream& output, size_t indent);
    void generate(const wasm::op_label& instruction, std::ostream& output, size_t indent);

    void generate_export(const wasm::export_item& export_item, std::ostream& output, size_t indent);
};
