#include "ir.hpp"
#include <algorithm>
#include <format>
#include <iomanip>
#include <sstream>

std::string repr_wasm_type(wasm_type typ)
{
    switch (typ) {
        case wasm_type::none:
            return "";
        case wasm_type::u8:
        case wasm_type::u16:
        case wasm_type::u32:
        case wasm_type::i8:
        case wasm_type::i16:
        case wasm_type::i32:
            return "i32";
        case wasm_type::u64:
        case wasm_type::i64:
            return "i64";
        case wasm_type::f32:
            return "f32";
        case wasm_type::f64:
            return "f64";
    }
}

std::string sign_wasm_type(wasm_type typ)
{
    switch (typ) {
        case wasm_type::none:
            return "";
        case wasm_type::i8:
        case wasm_type::i16:
        case wasm_type::i32:
        case wasm_type::i64:
        case wasm_type::f32:
        case wasm_type::f64:
            return "s";
        case wasm_type::u8:
        case wasm_type::u16:
        case wasm_type::u32:
        case wasm_type::u64:
            return "u";
    }
}

std::string size_wasm_type(wasm_type typ)
{
    switch (typ) {
        case wasm_type::none:
        case wasm_type::u32:
        case wasm_type::i32:
        case wasm_type::f32:
        case wasm_type::u64:
        case wasm_type::i64:
        case wasm_type::f64:
            return "";
        case wasm_type::i8:
        case wasm_type::u8:
            return "8";
        case wasm_type::i16:
        case wasm_type::u16:
            return "16";
    }
}

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

wasm_statement::~wasm_statement()
{
}

wasm_instruction::wasm_instruction(wasm_op op)
    : op(op)
{
}

void wasm_instruction::dump(std::ostream &output, size_t indent) const
{
    std::string ind(indent, ' ');
    output << ind;
    switch (op)
    {
        case wasm_op::nop:
            output << "nop\n";
            break;
        case wasm_op::unreachable:
            output << "unreachable\n";
            break;
        case wasm_op::drop:
            output << "drop\n";
            break;
        default:
            break;
    }
}

wasm_op_index::wasm_op_index(wasm_op op, const std::string var_name, uint32_t index)
    : wasm_instruction(op), name(var_name), index(index)
{
}

void wasm_op_index::dump(std::ostream &output, size_t indent) const
{
    std::string ind(indent, ' ');
    output << ind;
    switch (op)
    {
        case wasm_op::local_get:
            output << "local.get ";
            break;
        case wasm_op::local_set:
            output << "local.set ";
            break;
        case wasm_op::local_tee:
            output << "local.tee ";
            break;
        case wasm_op::global_get:
            output << "global.get ";
            break;
        case wasm_op::global_set:
            output << "global.set ";
            break;
        default:
            break;
    }
    output << "$" << name << "\n";
}

wasm_op_type::wasm_op_type(wasm_op op, wasm_type type)
    : wasm_instruction(op), value_type(type)
{
}

void wasm_op_type::dump(std::ostream &output, size_t indent) const
{
    std::string ind(indent, ' ');
    output << ind;

    output << repr_wasm_type(value_type);
    switch (op)
    {
        case wasm_op::iadd:
            output << ".add\n";
            break;
        case wasm_op::isub:
            output << ".sub\n";
            break;
        case wasm_op::imul:
            output << ".mul\n";
            break;
        case wasm_op::eqz:
            output << ".eqz\n";
            break;
        default:
            break;
    }
}

wasm_op_type_sign::wasm_op_type_sign(wasm_op op, wasm_type type)
    : wasm_instruction(op), value_type(type)
{
}

void wasm_op_type_sign::dump(std::ostream &output, size_t indent) const
{
    std::string ind(indent, ' ');
    output << ind;

    output << repr_wasm_type(value_type);
    switch (op)
    {
        case wasm_op::idiv:
            output << ".div_";
            break;
        case wasm_op::irem:
            output << ".rem_";
            break;
        default:
            break;
    }
    output << sign_wasm_type(value_type) << "\n";
}

wasm_op_type_value::wasm_op_type_value(wasm_op op, wasm_type type, uint64_t value)
    : wasm_instruction(op), value_type(type), value(value)
{
}

void wasm_op_type_value::dump(std::ostream &output, size_t indent) const
{
    std::string ind(indent, ' ');
    output << ind;

    output << repr_wasm_type(value_type);
    switch (op)
    {
        case wasm_op::typ_const:
            output << ".const ";
            break;
        default:
            break;
    }
    output << value << "\n";
}

wasm_op_align_offset::wasm_op_align_offset(wasm_op op, wasm_type type, uint32_t alignment, uint64_t offset)
    : wasm_instruction(op), value_type(type), alignment(alignment), offset(offset)
{
}

void wasm_op_align_offset::dump(std::ostream &output, size_t indent) const
{
    std::string ind(indent, ' ');
    output << ind;

    std::string sx = sign_wasm_type(value_type);
    std::string sz = size_wasm_type(value_type);

    std::string suffix = "";
    std::string offset_suffix = "";
    if (!sz.empty())
    {
        suffix = std::format("{}_{}", sz, sx);
    }
    if (offset>0)
    {
        offset_suffix = std::format(" offset={}", offset);
    }

    output << repr_wasm_type(value_type);
    switch (op)
    {
        case wasm_op::load:
            output << ".load" << suffix << offset_suffix;
            break;
        case wasm_op::store:
            output << ".store" << sz << offset_suffix;
            break;
        default:
            break;
    }
    output << "\n";
}

wasm_op_label::wasm_op_label(wasm_op op, wasm_branch_label label)
    : wasm_instruction(op), label(label)
{
}

void wasm_op_label::dump(std::ostream &output, size_t indent) const
{
    std::string ind(indent, ' ');
    output << ind;

    switch (op)
    {
        case wasm_op::br:
            output << "br $";
            break;
        case wasm_op::br_if:
            output << "br_if $";
            break;
        default:
            break;
    }
    output << label.repr() << "\n";
}

wasm_op_func::wasm_op_func(wasm_op op, wasm_function_ref function)
    : wasm_instruction(op), function(function)
{
}

void wasm_op_func::dump(std::ostream &output, size_t indent) const
{
    std::string ind(indent, ' ');
    output << ind;

    switch (op)
    {
        case wasm_op::call:
            output << "call $";
            break;
        default:
            break;
    }
    output << function.name() << "\n";
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

void wasm_if_block::dump(std::ostream &output, size_t indent) const
{
    std::string ind(indent, ' ');
    output << ind << "if";
    if (return_type != wasm_type::none)
    {
        output << " (result " << repr_wasm_type(return_type) << ")\n";
    }
    else
    {
        output << "\n";
    }
    then_block->dump(output, indent+2);
    if (else_block->inst_count() > 0)
    {
        output << ind << "else\n";
        else_block->dump(output, indent+2);
    }
    output << ind << "end\n";
}

wasm_block::wasm_block()
{
}

void wasm_block::dump(std::ostream &output, size_t indent) const
{
    for (const auto& inst : instructions)
    {
        inst->dump(output, indent);
    }
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
    auto block = std::make_unique<wasm_internal_block>(return_type);
    wasm_block& new_block = *block.get();
    instructions.push_back(std::move(block));
    return new_block;
}

wasm_block& wasm_block::loop(wasm_type return_type)
{
    auto block = std::make_unique<wasm_loop_block>(return_type);
    wasm_block& new_block = *block.get();
    instructions.push_back(std::move(block));
    return new_block;
}

std::pair<wasm_block&, wasm_block&> wasm_block::if_block(wasm_type return_type)
{
    auto if_block = std::make_unique<wasm_if_block>(return_type);
    auto blocks = if_block->blocks();
    instructions.push_back(std::move(if_block));
    return blocks;
}

void wasm_block::local_get(const char* variable_name)
{
    instructions.push_back(std::make_unique<wasm_op_index>(wasm_op_index(wasm_op::local_get, variable_name, 0)));
}

void wasm_block::local_get(uint32_t index)
{
    instructions.push_back(std::make_unique<wasm_op_index>(wasm_op_index(wasm_op::local_get, "", index)));
}

void wasm_block::local_set(const char* variable_name)
{
    instructions.push_back(std::make_unique<wasm_op_index>(wasm_op_index(wasm_op::local_set, variable_name, 0)));
}

void wasm_block::global_get(const char* variable_name)
{
    instructions.push_back(std::make_unique<wasm_op_index>(wasm_op_index(wasm_op::global_get, variable_name, 0)));
}

void wasm_block::global_set(const char* variable_name)
{
    instructions.push_back(std::make_unique<wasm_op_index>(wasm_op_index(wasm_op::global_set, variable_name, 0)));
}

void wasm_block::const_val(wasm_type type, uint64_t value)
{
    instructions.push_back(std::make_unique<wasm_op_type_value>(wasm_op_type_value(wasm_op::typ_const, type, value)));
}

void wasm_block::drop()
{
    instructions.push_back(std::make_unique<wasm_instruction>(wasm_instruction(wasm_op::drop)));
}

void wasm_block::add(wasm_type type)
{
    instructions.push_back(std::make_unique<wasm_op_type>(wasm_op_type(wasm_op::iadd, type)));
}

void wasm_block::div(wasm_type type)
{
    instructions.push_back(std::make_unique<wasm_op_type_sign>(wasm_op_type_sign(wasm_op::idiv, type)));
}

void wasm_block::mod(wasm_type type)
{
    instructions.push_back(std::make_unique<wasm_op_type_sign>(wasm_op_type_sign(wasm_op::irem, type)));
}

void wasm_block::mul(wasm_type type)
{
    instructions.push_back(std::make_unique<wasm_op_type>(wasm_op_type(wasm_op::imul, type)));
}

void wasm_block::sub(wasm_type type)
{
    instructions.push_back(std::make_unique<wasm_op_type>(wasm_op_type(wasm_op::isub, type)));
}

void wasm_block::load(wasm_type type, size_t offset)
{
    instructions.push_back(std::make_unique<wasm_op_align_offset>(wasm_op_align_offset(wasm_op::load, type, 0, offset)));
}

void wasm_block::store(wasm_type type, size_t offset)
{
    instructions.push_back(std::make_unique<wasm_op_align_offset>(wasm_op_align_offset(wasm_op::store, type, 0, offset)));
}

void wasm_block::eqz(wasm_type type)
{
    instructions.push_back(std::make_unique<wasm_op_type>(wasm_op_type(wasm_op::eqz, type)));
}

void wasm_block::br(wasm_branch_label branch_label)
{
    instructions.push_back(std::make_unique<wasm_op_label>(wasm_op_label(wasm_op::br, branch_label)));
}

void wasm_block::br_if(wasm_branch_label branch_label)
{
    instructions.push_back(std::make_unique<wasm_op_label>(wasm_op_label(wasm_op::br_if, branch_label)));
}

void wasm_block::call(const char* function_name)
{
    instructions.push_back(std::make_unique<wasm_op_func>(wasm_op_func(wasm_op::call, wasm_function_ref(function_name))));
}

wasm_internal_block::wasm_internal_block(wasm_type return_type)
: return_type(return_type)
{
}

void wasm_internal_block::dump(std::ostream &output, size_t indent) const
{
    std::string ind(indent, ' ');
    output << ind << "block $" << block_label.repr() << "\n";

    wasm_block::dump(output, indent+2);

    output << ind << "end\n";
}

wasm_loop_block::wasm_loop_block(wasm_type return_type)
: return_type(return_type)
{
}

void wasm_loop_block::dump(std::ostream &output, size_t indent) const
{
    std::string ind(indent, ' ');
    output << ind << "loop $" << block_label.repr() << "\n";

    wasm_block::dump(output, indent+2);

    output << ind << "end\n";
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

void exportable::dump_export(std::ostream &output, size_t indent) const
{
    std::string ind(indent, ' ');
    output << ind << "(export \"" << export_name << "\" (";
    output << export_type << " ";
    if (!name.empty())
    {
        output << "$" << name;
    }
    else
    {
        output << index;
    }
    output << "))\n";
}

wasm_function::wasm_function(const char* name, wasm_type return_type, arguments_type arguments)
    : exportable(name, "func"), return_type(return_type), arguments(arguments)
{
}

void wasm_function::dump(std::ostream &output, size_t indent) const
{
    std::string ind(indent, ' ');
    output << ind << "(func $" << name;
    for (const auto& arg : arguments)
    {
        output << " (param $" << arg.first << " " << repr_wasm_type(arg.second) << ")";
    }
    if (return_type != wasm_type::none)
    {
        output << " (result " << repr_wasm_type(return_type) << ")";
    }

    if (!is_imported())
    {
        // if the function is imported, we only dump the function
        // signature without a body and keep everything on a single line
        output << "\n";

        for (auto& local_var : locals)
        {
            output << ind << ind << "(local $" << local_var.first << " " << 
            repr_wasm_type(local_var.second) << ")\n";
        }

        function_body.dump(output, indent + 2);
    }

    output << ind << ")";
    if (!is_imported())
    {
        // for imported functions we want to keep everything on a
        // single line; the module dumping will add the newline
        output << "\n";
    }
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

void wasm_memory::dump(std::ostream &output, size_t indent) const
{
    std::string ind(2, ' ');
    output << ind << "(memory " << initial_block_count << ")\n";
}

wasm_data_section::wasm_data_section(size_t offset)
: init_offset(offset), offset(offset), data_buffer()
{
}

void wasm_data_section::dump(std::ostream &output, size_t indent) const
{
    std::string ind(2, ' ');
    output << ind << "(data (i32.const " << init_offset << ") \"";
    for (const char& ch : data_buffer) {
        switch (ch) {
            case '\\':
                output << "\\\\";
                break;
            case '\'':
                output << "\\\'";
                break;
            case '\"':
                output << "\\\"";
                break;
            case '\t':
                output << "\\t";
                break;
            case '\r':
                output << "\\r";
                break;
            case '\n':
                output << "\\n";
                break;
            default:
                if (ch>='\x20')
                {
                    output << std::string(1, ch);
                }
                else
                {
                    std::ostringstream oss;
                    oss << "\\" << std::uppercase << std::hex << std::setw(2) << std::setfill('0') 
                        << ((unsigned int)ch & 0xff);
                    output << oss.str();
                }
        }
    }
    output << "\")\n";
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

void wasm_module::dump(std::ostream &output) const
{
    std::string ind(2, ' ');

    output << "(module\n";
    for (const auto& func : functions)
    {
        if (!func.second.is_imported())
        {
            continue;
        }
        output << ind << "(import ";
        output << "\"" << func.second.ns << "\" ";
        output << "\"" << func.second.import_name << "\" ";
        func.second.dump(output, 0);
        output << ")\n";
    }
    for (const auto& data_section : data_sections)
    {
        data_section.dump(output, 2);
    }
    for (const auto& mem : memories)
    {
        mem.dump(output, 2);
    }
    for (const auto& global : globals)
    {
        output << ind << "(global $" << global.name;
        if (global.access==access_type::mut) {
            output << " (mut ";
            output << repr_wasm_type(global.typ);
            output << ")";
        }
        else
        {
            output << " " << repr_wasm_type(global.typ);
        }
        output << " (";
        output << repr_wasm_type(global.typ) << ".const ";
        output << global.initvalue;
        output << "))\n";
    }
    for (const auto& exp : exports)
    {
        exp.get().dump_export(output, 2);
    }
    for (const auto& func : functions)
    {
        if (func.second.is_imported())
        {
            continue;
        }
        func.second.dump(output, 2);
    }
    output << ")\n";
}