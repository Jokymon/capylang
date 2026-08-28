#include "wasm_ir.hpp"
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

bool wasm_branch_label::operator==(const wasm_branch_label& other)
{
    return this->label_representation == other.label_representation;
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

wasm_op_base::wasm_op_base(wasm_op op)
: op(op)
{
}

wasm_op_index::wasm_op_index(wasm_op op, const std::string var_name)
: wasm_op_base(op)
, name(var_name)
{
}

wasm_op_type::wasm_op_type(wasm_op op, wasm_type type)
: wasm_op_base(op)
, value_type(type)
{
}

wasm_op_type_sign::wasm_op_type_sign(wasm_op op, wasm_type type)
: wasm_op_base(op)
, value_type(type)
{
}

wasm_op_type_value::wasm_op_type_value(wasm_op op, wasm_type type, int64_t value)
: wasm_op_base(op)
, value_type(type)
, value(value)
{
}

wasm_op_align_offset::wasm_op_align_offset(wasm_op op, wasm_type type, uint32_t alignment, uint64_t offset)
: wasm_op_base(op)
, value_type(type)
, alignment(alignment)
, offset(offset)
{
}

wasm_op_label::wasm_op_label(wasm_op op, wasm_branch_label label)
: wasm_op_base(op)
, label(label)
{
}

wasm_op_func::wasm_op_func(wasm_op op, wasm_function_ref function)
: wasm_op_base(op)
, function(function)
{
}

wasm_if_block::wasm_if_block(wasm_type return_type, wasm_block* enclosing_block)
: return_type(return_type)
, then_block(std::make_unique<wasm_block>())
, else_block(std::make_unique<wasm_block>())
{
    then_block->enclosing_block = enclosing_block;
    else_block->enclosing_block = enclosing_block;
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

namespace wasm_builder
{
    wasm_block& block(wasm_expr& expr, wasm_block* parent, wasm_type return_type)
    {
        auto block = std::make_unique<wasm_instruction>(
            wasm_block{
                wasm_block::t_block, return_type, wasm_branch_label(), parent
            }
        );
        wasm_block& new_block = std::get<wasm_block>(*block);
        expr.push_back(std::move(block));
        return new_block;
    }

    wasm_block& loop(wasm_expr& expr, wasm_block* parent, wasm_type return_type)
    {
        auto block = std::make_unique<wasm_instruction>(
            wasm_block{
                wasm_block::t_loop, return_type, wasm_branch_label(), parent
            }
        );
        wasm_block& new_block = std::get<wasm_block>(*block);
        expr.push_back(std::move(block));
        return new_block;
    }

    std::pair<wasm_block&, wasm_block&> if_block(wasm_expr& expr, wasm_block* parent, wasm_type return_type)
    {
        auto if_block = std::make_unique<wasm_instruction>(wasm_if_block{return_type, parent});
        auto blocks = std::get<wasm_if_block>(*if_block).blocks();
        expr.push_back(std::move(if_block));
        return blocks;
    }

    void local_get(wasm_expr& expr, const char* variable_name)
    {
        expr.push_back(std::make_unique<wasm_instruction>(wasm_op_index(wasm_op::local_get, variable_name)));
    }

    void local_set(wasm_expr& expr, const char* variable_name)
    {
        expr.push_back(std::make_unique<wasm_instruction>(wasm_op_index(wasm_op::local_set, variable_name)));
    }

    void global_get(wasm_expr& expr, const char* variable_name)
    {
        expr.push_back(std::make_unique<wasm_instruction>(wasm_op_index(wasm_op::global_get, variable_name)));
    }

    void global_set(wasm_expr& expr, const char* variable_name)
    {
        expr.push_back(std::make_unique<wasm_instruction>(wasm_op_index(wasm_op::global_set, variable_name)));
    }

    void const_val(wasm_expr& expr, wasm_type type, int64_t value)
    {
        expr.push_back(std::make_unique<wasm_instruction>(wasm_op_type_value(wasm_op::typ_const, type, value)));
    }

    void drop(wasm_expr& expr)
    {
        expr.push_back(std::make_unique<wasm_instruction>(wasm_op_base(wasm_op::drop)));
    }

    void add(wasm_expr& expr, wasm_type type)
    {
        expr.push_back(std::make_unique<wasm_instruction>(wasm_op_type(wasm_op::iadd, type)));
    }

    void and_instr(wasm_expr& expr, wasm_type type)
    {
        expr.push_back(std::make_unique<wasm_instruction>(wasm_op_type(wasm_op::iand, type)));
    }

    void div(wasm_expr& expr, wasm_type type)
    {
        expr.push_back(std::make_unique<wasm_instruction>(wasm_op_type_sign(wasm_op::idiv, type)));
    }

    void mod(wasm_expr& expr, wasm_type type)
    {
        expr.push_back(std::make_unique<wasm_instruction>(wasm_op_type_sign(wasm_op::irem, type)));
    }

    void mul(wasm_expr& expr, wasm_type type)
    {
        expr.push_back(std::make_unique<wasm_instruction>(wasm_op_type(wasm_op::imul, type)));
    }

    void or_instr(wasm_expr& expr, wasm_type type)
    {
        expr.push_back(std::make_unique<wasm_instruction>(wasm_op_type(wasm_op::ior, type)));
    }

    void sub(wasm_expr& expr, wasm_type type)
    {
        expr.push_back(std::make_unique<wasm_instruction>(wasm_op_type(wasm_op::isub, type)));
    }

    void shl(wasm_expr& expr, wasm_type type)
    {
        expr.push_back(std::make_unique<wasm_instruction>(wasm_op_type(wasm_op::ishl, type)));
    }

    void shr(wasm_expr& expr, wasm_type type)
    {
        expr.push_back(std::make_unique<wasm_instruction>(wasm_op_type_sign(wasm_op::ishr, type)));
    }

    void load(wasm_expr& expr, wasm_type type, size_t offset)
    {
        expr.push_back(std::make_unique<wasm_instruction>(wasm_op_align_offset(wasm_op::load, type, 0, offset)));
    }

    void store(wasm_expr& expr, wasm_type type, size_t offset)
    {
        expr.push_back(std::make_unique<wasm_instruction>(wasm_op_align_offset(wasm_op::store, type, 0, offset)));
    }

    void eq(wasm_expr& expr, wasm_type type)
    {
        expr.push_back(std::make_unique<wasm_instruction>(wasm_op_type(wasm_op::eq, type)));
    }

    void ne(wasm_expr& expr, wasm_type type)
    {
        expr.push_back(std::make_unique<wasm_instruction>(wasm_op_type(wasm_op::ne, type)));
    }

    void eqz(wasm_expr& expr, wasm_type type)
    {
        expr.push_back(std::make_unique<wasm_instruction>(wasm_op_type(wasm_op::eqz, type)));
    }

    void lt(wasm_expr& expr, wasm_type type)
    {
        expr.push_back(std::make_unique<wasm_instruction>(wasm_op_type_sign(wasm_op::ilt, type)));
    }

    void lte(wasm_expr& expr, wasm_type type)
    {
        expr.push_back(std::make_unique<wasm_instruction>(wasm_op_type_sign(wasm_op::ilte, type)));
    }

    void gt(wasm_expr& expr, wasm_type type)
    {
        expr.push_back(std::make_unique<wasm_instruction>(wasm_op_type_sign(wasm_op::igt, type)));
    }

    void gte(wasm_expr& expr, wasm_type type)
    {
        expr.push_back(std::make_unique<wasm_instruction>(wasm_op_type_sign(wasm_op::igte, type)));
    }

    void br(wasm_expr& expr, wasm_branch_label branch_label)
    {
        expr.push_back(std::make_unique<wasm_instruction>(wasm_op_label(wasm_op::br, branch_label)));
    }

    void br_if(wasm_expr& expr, wasm_branch_label branch_label)
    {
        expr.push_back(std::make_unique<wasm_instruction>(wasm_op_label(wasm_op::br_if, branch_label)));
    }

    void call(wasm_expr& expr, const char* function_name)
    {
        expr.push_back(std::make_unique<wasm_instruction>(wasm_op_func(wasm_op::call, wasm_function_ref(function_name))));
    }

    void ret(wasm_expr& expr)
    {
        expr.push_back(std::make_unique<wasm_instruction>(wasm_op_base(wasm_op::ret)));
    }

    void memory_size(wasm_expr& expr)
    {
        expr.push_back(std::make_unique<wasm_instruction>(wasm_op_base(wasm_op::memory_size)));
    }

    void memory_grow(wasm_expr& expr)
    {
        expr.push_back(std::make_unique<wasm_instruction>(wasm_op_base(wasm_op::memory_grow)));
    }

    void memory_fill(wasm_expr& expr)
    {
        expr.push_back(std::make_unique<wasm_instruction>(wasm_op_base(wasm_op::memory_fill)));
    }

    void unreachable(wasm_expr& expr)
    {
        expr.push_back(std::make_unique<wasm_instruction>(wasm_op_base(wasm_op::unreachable)));
    }
}

wasm_block wasm_block::create_init_block()
{
    return wasm_block{
        wasm_block_type::t_plain,
        wasm_type::none,
        wasm_branch_label(),
        nullptr
    };
}

exportable::exportable(index_type index, wasm_extern_index export_type)
: name()
, index(index)
, export_type(export_type)
{
}

void importable::import_from(const char* ns, const char* import_name)
{
    this->ns = ns;
    this->import_name = import_name;
}

bool importable::is_imported() const
{
    return !ns.empty();
}

wasm_function::wasm_function(index_type index, const std::string& name, wasm_type return_type, arguments_type arguments)
: exportable(index, wasm_extern_index::funcidx)
, return_type(return_type)
, arguments(arguments)
{
    this->name = name;

    size_t arg_index = 0;
    for (const auto& arg : arguments)
    {
        locals_map[arg.name] = arg_index++;
    }
}

void wasm_function::allocate_local(const char* name, wasm_type var_type)
{
    size_t index = arguments.size() + locals.size();
    locals.push_back({name, var_type});
    locals_map[name] = index;
}

wasm_memory::wasm_memory(index_type index, size_t initial_block_count)
: exportable(index, wasm_extern_index::memidx)
, initial_block_count(initial_block_count)
{
}

wasm_data_section::wasm_data_section(size_t offset)
: init_offset(offset)
, offset(offset)
, data_buffer()
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
    return memories[memories.size() - 1];
}

wasm_data_section& wasm_module::create_data_section(size_t init_offset)
{
    data_sections.push_back(wasm_data_section{init_offset});
    return data_sections.back();
}

wasm_global& wasm_module::create_global(const char* name, wasm_type g_type, access_type access)
{
    size_t index = globals.size();
    globals.push_back(wasm_global{name, access, g_type, {}});
    globals_map[name] = index;

    return globals.back();
}

wasm_function& wasm_module::create_function(const char* name, wasm_type return_type, arguments_type arguments)
{
    size_t function_index = functions.size();
    functions.emplace_back(wasm_function{function_index, name, return_type, arguments});
    functions_map[std::string(name)] = function_index;
    return functions.back();
}

void wasm_module::append_data_section(wasm_data_section& data)
{
    data_sections.push_back(data);
}

void wasm_module::export_as(const char* export_name, exportable& object)
{
    exports.push_back({
        .export_name = export_name,
        .name = object.name,
        .index = object.index,
        .export_type = object.export_type,
    });
}
