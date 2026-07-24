#include "wat_generator.hpp"
#include "tools.hpp"
#include <iomanip>
#include <sstream>
#include <variant>

std::string repr_wasm_type(wasm_type typ)
{
    switch (typ)
    {
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
        default:
            CAPY_FAIL("Unexpected wasm_type in repr_wasm_type");
            return "";
    }
}

std::string repr_wasm_extern_type(wasm_extern_index index)
{
    switch (index)
    {
        case wasm_extern_index::funcidx:
            return "func";
        case wasm_extern_index::tableidx:
            return "table";
        case wasm_extern_index::memidx:
            return "memory";
        case wasm_extern_index::globalidx:
            return "global";
        case wasm_extern_index::tagidx:
            return "tag";
    }
}

std::string sign_wasm_type(wasm_type typ)
{
    switch (typ)
    {
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
    switch (typ)
    {
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

void wat_generator::generate(const wasm_module& module, std::ostream& output)
{
    std::string ind(2, ' ');

    output << "(module\n";
    for (const auto& func : module.functions)
    {
        if (!func.is_imported())
        {
            continue;
        }
        output << ind << "(import ";
        output << "\"" << func.ns << "\" ";
        output << "\"" << func.import_name << "\" ";
        generate(func, output, 0);
        output << ")\n";
    }
    for (const auto& data_section : module.data_sections)
    {
        generate(data_section, output, 2);
    }
    for (const auto& mem : module.memories)
    {
        generate(mem, output, 2);
    }
    for (const auto& global : module.globals)
    {
        output << ind << "(global $" << global.name;
        if (global.access == wasm_module::access_type::mut)
        {
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
    for (const auto& exp : module.exports)
    {
        generate_export(exp, output, 2);
    }
    for (const auto& func : module.functions)
    {
        if (func.is_imported())
        {
            continue;
        }
        generate(func, output, 2);
    }
    output << ")\n";
}

void wat_generator::generate(const wasm_function& function, std::ostream& output, size_t indent)
{
    std::string ind(indent, ' ');
    output << ind << "(func $" << function.name;
    for (const auto& arg : function.arguments)
    {
        output << " (param $" << arg.name << " " << repr_wasm_type(arg.type) << ")";
    }
    if (function.return_type != wasm_type::none)
    {
        output << " (result " << repr_wasm_type(function.return_type) << ")";
    }

    if (!function.is_imported())
    {
        // if the function is imported, we only generate the function
        // signature without a body and keep everything on a single line
        output << "\n";

        for (auto& local_var : function.locals)
        {
            output << ind << ind << "(local $" << local_var.name << " " << repr_wasm_type(local_var.type) << ")\n";
        }

        generate(function.function_body, output, indent + 2);
    }

    output << ind << ")";
    if (!function.is_imported())
    {
        // for imported functions we want to keep everything on a
        // single line; the module generation will add the newline
        output << "\n";
    }
}

void wat_generator::generate(const wasm_data_section& section, std::ostream& output, size_t indent)
{
    std::string ind(2, ' ');
    output << ind << "(data (i32.const " << section.init_offset << ") \"";
    for (const char& ch : section.data_buffer)
    {
        switch (ch)
        {
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
                if (ch >= '\x20')
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

void wat_generator::generate(const wasm_memory& memory, std::ostream& output, size_t indent)
{
    std::string ind(2, ' ');
    output << ind << "(memory " << memory.initial_block_count << ")\n";
}

void wat_generator::generate(const wasm_block& block, std::ostream& output, size_t indent)
{
    std::string ind(indent, ' ');
    switch (block.block_type)
    {
        case wasm_block::t_plain:
            break;
        case wasm_block::t_block:
            output << ind << "block $" << block.block_label.repr() << "\n";
            break;
        case wasm_block::t_loop:
            output << ind << "loop $" << block.block_label.repr() << "\n";
            break;
    }

    for (const auto& inst : block.instructions)
    {
        std::visit(
            [&, this](const auto& n)
            {
                this->generate(n, output, indent + (block.block_type != wasm_block::t_plain ? 2 : 0));
            },
            *inst
        );
    }

    switch (block.block_type)
    {
        case wasm_block::t_plain:
            break;
        case wasm_block::t_block:
        case wasm_block::t_loop:
            output << ind << "end\n";
            break;
    }
}

void wat_generator::generate(const wasm_if_block& block, std::ostream& output, size_t indent)
{
    std::string ind(indent, ' ');
    output << ind << "if";
    if (block.return_type != wasm_type::none)
    {
        output << " (result " << repr_wasm_type(block.return_type) << ")\n";
    }
    else
    {
        output << "\n";
    }
    generate(*block.then_block, output, indent + 2);
    if (block.else_block->inst_count() > 0)
    {
        output << ind << "else\n";
        generate(*block.else_block, output, indent + 2);
    }
    output << ind << "end\n";
}

void wat_generator::generate(const wasm_instruction& inst, std::ostream& output, size_t indent)
{
    std::string ind(indent, ' ');
    output << ind;
    switch (inst.op)
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
        case wasm_op::ret:
            output << "return\n";
            break;
        case wasm_op::memory_size:
            output << "memory.size\n";
            break;
        case wasm_op::memory_grow:
            output << "memory.grow\n";
            break;
        case wasm_op::memory_fill:
            output << "memory.fill\n";
            break;
        default:
            break;
    }
}

void wat_generator::generate(const wasm_op_index& inst, std::ostream& output, size_t indent)
{
    std::string ind(indent, ' ');
    output << ind;
    switch (inst.op)
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
    output << "$" << inst.name << "\n";
}

void wat_generator::generate(const wasm_op_type& inst, std::ostream& output, size_t indent)
{
    std::string ind(indent, ' ');
    output << ind;

    output << repr_wasm_type(inst.value_type);
    switch (inst.op)
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
        case wasm_op::iand:
            output << ".and\n";
            break;
        case wasm_op::ior:
            output << ".or\n";
            break;
        case wasm_op::ishl:
            output << ".shl\n";
            break;
        case wasm_op::eq:
            output << ".eq\n";
            break;
        case wasm_op::ne:
            output << ".ne\n";
            break;
        case wasm_op::eqz:
            output << ".eqz\n";
            break;
        default:
            break;
    }
}

void wat_generator::generate(const wasm_op_type_sign& inst, std::ostream& output, size_t indent)
{
    std::string ind(indent, ' ');
    output << ind;

    output << repr_wasm_type(inst.value_type);
    switch (inst.op)
    {
        case wasm_op::idiv:
            output << ".div_";
            break;
        case wasm_op::irem:
            output << ".rem_";
            break;
        case wasm_op::ishr:
            output << ".shr_";
            break;
        case wasm_op::ilt:
            output << ".lt_";
            break;
        case wasm_op::ilte:
            output << ".le_";
            break;
        case wasm_op::igt:
            output << ".gt_";
            break;
        case wasm_op::igte:
            output << ".ge_";
            break;
        default:
            break;
    }
    output << sign_wasm_type(inst.value_type) << "\n";
}

void wat_generator::generate(const wasm_op_type_value& inst, std::ostream& output, size_t indent)
{
    std::string ind(indent, ' ');
    output << ind;

    output << repr_wasm_type(inst.value_type);
    switch (inst.op)
    {
        case wasm_op::typ_const:
            output << ".const ";
            break;
        default:
            break;
    }
    output << inst.value << "\n";
}

void wat_generator::generate(const wasm_op_align_offset& inst, std::ostream& output, size_t indent)
{
    std::string ind(indent, ' ');
    output << ind;

    std::string sx = sign_wasm_type(inst.value_type);
    std::string sz = size_wasm_type(inst.value_type);

    std::string suffix = "";
    std::string offset_suffix = "";
    if (!sz.empty())
    {
        suffix = std::format("{}_{}", sz, sx);
    }
    if (inst.offset > 0)
    {
        offset_suffix = std::format(" offset={}", inst.offset);
    }

    output << repr_wasm_type(inst.value_type);
    switch (inst.op)
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

void wat_generator::generate(const wasm_op_label& inst, std::ostream& output, size_t indent)
{
    std::string ind(indent, ' ');
    output << ind;

    switch (inst.op)
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
    output << inst.label.repr() << "\n";
}

void wat_generator::generate(const wasm_op_func& inst, std::ostream& output, size_t indent)
{
    std::string ind(indent, ' ');
    output << ind;

    switch (inst.op)
    {
        case wasm_op::call:
            output << "call $";
            break;
        default:
            break;
    }
    output << inst.function.name() << "\n";
}

void wat_generator::generate_export(const wasm_export& exp, std::ostream& output, size_t indent)
{
    std::string ind(indent, ' ');
    output << ind << "(export \"" << exp.export_name << "\" (";
    output << repr_wasm_extern_type(exp.export_type) << " ";
    if (!exp.name.empty())
    {
        output << "$" << exp.name;
    }
    else
    {
        output << exp.index;
    }
    output << "))\n";
}
