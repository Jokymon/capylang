#include "ir.hpp"
#include <algorithm>
#include <format>

wasm_branch_label::wasm_branch_label()
{
    label_representation = std::format("block_{}", label_index++);
}

std::string wasm_branch_label::repr() const
{
    return label_representation;
}

int wasm_branch_label::label_index = 0;

wasm_function_ref::wasm_function_ref(const std::string& name)
: name_(name)
{
}

std::string wasm_function_ref::name() const
{
    return name_;
}

wasm_instruction::wasm_instruction(wasm_op op)
    : op(op)
{
}

wasm_op_index::wasm_op_index(wasm_op op, const std::string var_name, uint32_t index)
    : wasm_instruction(op), name(var_name), index(index)
{
}

wasm_op_type::wasm_op_type(wasm_op op, wasm_type type)
    : wasm_instruction(op), value_type(type)
{
}

wasm_op_type_sign::wasm_op_type_sign(wasm_op op, wasm_type type)
    : wasm_instruction(op), value_type(type)
{
}

wasm_op_type_value::wasm_op_type_value(wasm_op op, wasm_type type, uint64_t value)
    : wasm_instruction(op), value_type(type), value(value)
{
}

wasm_op_align_offset::wasm_op_align_offset(wasm_op op, wasm_type type, uint32_t alignment, uint64_t offset)
    : wasm_instruction(op), value_type(type), alignment(alignment), offset(offset)
{
}

wasm_op_label::wasm_op_label(wasm_op op, wasm_branch_label label)
    : wasm_instruction(op), label(label)
{
}

wasm_op_func::wasm_op_func(wasm_op op, wasm_function_ref function)
    : wasm_instruction(op), function(function)
{
}

wasm_if_block::wasm_if_block(wasm_type return_type)
: return_type(return_type), then_block(std::make_unique<wasm_block>()), 
else_block(std::make_unique<wasm_block>())
{
}

std::pair<wasm_block&, wasm_block&> wasm_if_block::blocks()
{
    return std::pair<wasm_block&, wasm_block&>(*then_block, *else_block);
}

size_t wasm_block::inst_count() const
{
    return instructions.size();
}

wasm_branch_label wasm_block::label() const
{
    return block_label;
}

wasm_block& wasm_block::block(wasm_type return_type)
{
    auto block = std::make_unique<wasm_statement>(wasm_internal_block{return_type});
    wasm_block& new_block = std::get<wasm_internal_block>(*block);
    instructions.push_back(std::move(block));
    return new_block;
}

wasm_block& wasm_block::loop(wasm_type return_type)
{
    auto block = std::make_unique<wasm_statement>(wasm_loop_block{return_type});
    wasm_block& new_block = std::get<wasm_loop_block>(*block);
    instructions.push_back(std::move(block));
    return new_block;
}

std::pair<wasm_block&, wasm_block&> wasm_block::if_block(wasm_type return_type)
{
    auto if_block = std::make_unique<wasm_statement>(wasm_if_block{return_type});
    auto blocks = std::get<wasm_if_block>(*if_block).blocks();
    instructions.push_back(std::move(if_block));
    return blocks;
}

void wasm_block::local_get(const char* variable_name)
{
    instructions.push_back(std::make_unique<wasm_statement>(wasm_op_index(wasm_op::local_get, variable_name, 0)));
}

void wasm_block::local_get(uint32_t index)
{
    instructions.push_back(std::make_unique<wasm_statement>(wasm_op_index(wasm_op::local_get, "", index)));
}

void wasm_block::local_set(const char* variable_name)
{
    instructions.push_back(std::make_unique<wasm_statement>(wasm_op_index(wasm_op::local_set, variable_name, 0)));
}

void wasm_block::global_get(const char* variable_name)
{
    instructions.push_back(std::make_unique<wasm_statement>(wasm_op_index(wasm_op::global_get, variable_name, 0)));
}

void wasm_block::global_set(const char* variable_name)
{
    instructions.push_back(std::make_unique<wasm_statement>(wasm_op_index(wasm_op::global_set, variable_name, 0)));
}

void wasm_block::const_val(wasm_type type, uint64_t value)
{
    instructions.push_back(std::make_unique<wasm_statement>(wasm_op_type_value(wasm_op::typ_const, type, value)));
}

void wasm_block::drop()
{
    instructions.push_back(std::make_unique<wasm_statement>(wasm_instruction(wasm_op::drop)));
}

void wasm_block::add(wasm_type type)
{
    instructions.push_back(std::make_unique<wasm_statement>(wasm_op_type(wasm_op::iadd, type)));
}

void wasm_block::div(wasm_type type)
{
    instructions.push_back(std::make_unique<wasm_statement>(wasm_op_type_sign(wasm_op::idiv, type)));
}

void wasm_block::mod(wasm_type type)
{
    instructions.push_back(std::make_unique<wasm_statement>(wasm_op_type_sign(wasm_op::irem, type)));
}

void wasm_block::mul(wasm_type type)
{
    instructions.push_back(std::make_unique<wasm_statement>(wasm_op_type(wasm_op::imul, type)));
}

void wasm_block::sub(wasm_type type)
{
    instructions.push_back(std::make_unique<wasm_statement>(wasm_op_type(wasm_op::isub, type)));
}

void wasm_block::load(wasm_type type, size_t offset)
{
    instructions.push_back(std::make_unique<wasm_statement>(wasm_op_align_offset(wasm_op::load, type, 0, offset)));
}

void wasm_block::store(wasm_type type, size_t offset)
{
    instructions.push_back(std::make_unique<wasm_statement>(wasm_op_align_offset(wasm_op::store, type, 0, offset)));
}

void wasm_block::eq(wasm_type type)
{
    instructions.push_back(std::make_unique<wasm_statement>(wasm_op_type(wasm_op::eq, type)));
}

void wasm_block::ne(wasm_type type)
{
    instructions.push_back(std::make_unique<wasm_statement>(wasm_op_type(wasm_op::ne, type)));
}

void wasm_block::eqz(wasm_type type)
{
    instructions.push_back(std::make_unique<wasm_statement>(wasm_op_type(wasm_op::eqz, type)));
}

void wasm_block::br(wasm_branch_label branch_label)
{
    instructions.push_back(std::make_unique<wasm_statement>(wasm_op_label(wasm_op::br, branch_label)));
}

void wasm_block::br_if(wasm_branch_label branch_label)
{
    instructions.push_back(std::make_unique<wasm_statement>(wasm_op_label(wasm_op::br_if, branch_label)));
}

void wasm_block::call(const char* function_name)
{
    instructions.push_back(std::make_unique<wasm_statement>(wasm_op_func(wasm_op::call, wasm_function_ref(function_name))));
}

wasm_internal_block::wasm_internal_block(wasm_type return_type)
: return_type(return_type)
{
}

wasm_loop_block::wasm_loop_block(wasm_type return_type)
: return_type(return_type)
{
}

exportable::exportable(const char* name, const char* export_type)
    : name(name), index(0), export_type(export_type)
{
}

exportable::exportable(index_type index, const char* export_type)
    : name(), index(index), export_type(export_type)
{
}

void exportable::export_as(const char* export_name)
{
    this->export_name = export_name;
}

wasm_function::wasm_function(const char* name, wasm_type return_type, arguments_type arguments)
    : exportable(name, "func"), return_type(return_type), arguments(arguments)
{
}

void wasm_function::allocate_local(const char* name, wasm_type var_type)
{
    locals.push_back({name, var_type});
}

wasm_block& wasm_function::body()
{
    return function_body;
}

bool wasm_function::is_imported() const
{
    return !ns.empty();
}

void wasm_function::import_from(const char* ns, const char* import_name)
{
    this->ns = ns;
    this->import_name = import_name;
}

wasm_memory::wasm_memory(index_type index, size_t initial_block_count)
    : exportable(index, "memory"), initial_block_count(initial_block_count)
{
}

wasm_data_section::wasm_data_section(size_t offset)
: init_offset(offset), offset(offset), data_buffer()
{
}

size_t wasm_data_section::allocate_data(const std::string& data)
{
    auto alloc_offset = offset;
    offset += data.size();
    data_buffer += data;
    return alloc_offset;
}

wasm_module::wasm_module()
{
}

wasm_memory& wasm_module::create_memory(size_t initial_block_count)
{
    memories.push_back(wasm_memory{memories.size(), initial_block_count});
    return memories[memories.size()-1];
}

wasm_data_section& wasm_module::create_data_section(size_t init_offset)
{
    data_sections.push_back(wasm_data_section{init_offset});
    return data_sections.back();
}

void wasm_module::create_global(const char* name, wasm_type g_type, wasm_module::access_type access, uint64_t initvalue)
{
    globals.push_back({name, access, g_type, initvalue});
}

wasm_function& wasm_module::create_function(const char* name, wasm_type return_type, arguments_type arguments)
{
    std::string sname(name);
    functions.emplace(sname, wasm_function{name, return_type, arguments});
    return functions.find(sname)->second;
}

void wasm_module::append_data_section(wasm_data_section& data)
{
    data_sections.push_back(data);
}

void wasm_module::export_as(const char* export_name, exportable& object)
{
    object.export_as(export_name);
    exports.push_back(object);
}
