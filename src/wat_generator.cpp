#include "wat_generator.hpp"

#include "tools.hpp"

#include <iomanip>
#include <sstream>
#include <variant>

namespace
{
    std::string repr_wasm_type(wasm::type_t type)
    {
        switch (type)
        {
            case wasm::type_t::None:
                return "";
            case wasm::type_t::U8:
            case wasm::type_t::U16:
            case wasm::type_t::U32:
            case wasm::type_t::I8:
            case wasm::type_t::I16:
            case wasm::type_t::I32:
                return "i32";
            case wasm::type_t::U64:
            case wasm::type_t::I64:
                return "i64";
            case wasm::type_t::F32:
                return "f32";
            case wasm::type_t::F64:
                return "f64";
        }
    }

    std::string repr_wasm_extern_type(wasm::extern_index_t index)
    {
        switch (index)
        {
            case wasm::extern_index_t::FuncIdx:
                return "func";
            case wasm::extern_index_t::TableIdx:
                return "table";
            case wasm::extern_index_t::MemIdx:
                return "memory";
            case wasm::extern_index_t::GlobalIdx:
                return "global";
            case wasm::extern_index_t::TagIdx:
                return "tag";
        }
    }

    std::string sign_wasm_type(wasm::type_t type)
    {
        switch (type)
        {
            case wasm::type_t::I8:
            case wasm::type_t::I16:
            case wasm::type_t::I32:
            case wasm::type_t::I64:
            case wasm::type_t::F32:
            case wasm::type_t::F64:
                return "s";
            case wasm::type_t::U8:
            case wasm::type_t::U16:
            case wasm::type_t::U32:
            case wasm::type_t::U64:
                return "u";
            case wasm::type_t::None:
                return "";
        }
    }

    std::string size_wasm_type(wasm::type_t type)
    {
        switch (type)
        {
            case wasm::type_t::I8:
            case wasm::type_t::U8:
                return "8";
            case wasm::type_t::I16:
            case wasm::type_t::U16:
                return "16";
            default:
                return "";
        }
    }

    bool is_imported(const wasm::importable& object)
    {
        return !object.ns.empty();
    }

    std::string indent(size_t count)
    {
        return std::string(count, ' ');
    }

    void generate_escaped_data(const std::string& data, std::ostream& output)
    {
        for (unsigned char byte : data)
        {
            switch (byte)
            {
                case '\\':
                    output << "\\\\";
                    break;
                case '"':
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
                    if (byte >= 0x20 && byte < 0x7f)
                    {
                        output.put(static_cast<char>(byte));
                    }
                    else
                    {
                        std::ostringstream escaped;
                        escaped << "\\" << std::uppercase << std::hex << std::setw(2)
                                << std::setfill('0') << static_cast<unsigned int>(byte);
                        output << escaped.str();
                    }
                    break;
            }
        }
    }
}

void wat_generator::generate(const wasm::module& module, std::ostream& output)
{
    output << "(module\n";

    for (const auto& function : module.functions)
    {
        if (!is_imported(*function))
            continue;
        output << "  (import \"" << function->ns << "\" \"" << function->import_name << "\" ";
        generate(*function, output, 0);
        output << ")\n";
    }

    for (const auto& data_section : module.data_sections)
        generate(*data_section, output, 2);
    for (const auto& memory : module.memories)
        generate(*memory, output, 2);
    for (const auto& global : module.globals)
        generate(*global, output, 2);
    for (const auto& export_item : module.exports)
        generate_export(*export_item, output, 2);

    for (const auto& function : module.functions)
    {
        if (is_imported(*function))
            continue;
        generate(*function, output, 2);
    }

    output << ")\n";
}

void wat_generator::generate(const wasm::function& function, std::ostream& output, size_t indentation)
{
    const auto ind = indent(indentation);
    output << ind << "(func";
    if (!function.name.empty())
        output << " $" << function.name;

    for (const auto& argument : function.arguments)
    {
        output << " (param $" << argument.name << " " << repr_wasm_type(argument.type) << ")";
    }
    if (function.return_type != wasm::type_t::None)
    {
        output << " (result " << repr_wasm_type(function.return_type) << ")";
    }

    if (is_imported(function))
    {
        output << ")";
        return;
    }

    output << "\n";
    for (const auto& local : function.locals)
    {
        output << indent(indentation + 2) << "(local $" << local.name << " "
               << repr_wasm_type(local.type) << ")\n";
    }

    generate(function.function_body, output, indentation + 2);
    output << ind << ")\n";
}

void wat_generator::generate(const wasm::data_section& section, std::ostream& output, size_t indentation)
{
    output << indent(indentation) << "(data (i32.const " << section.init_offset << ") \"";
    generate_escaped_data(section.data_buffer, output);
    output << "\")\n";
}

void wat_generator::generate(const wasm::memory& memory, std::ostream& output, size_t indentation)
{
    const auto ind = indent(indentation);
    if (is_imported(memory))
    {
        output << ind << "(import \"" << memory.ns << "\" \"" << memory.import_name << "\" ";
    }
    else
    {
        output << ind;
    }

    output << "(memory";
    if (!memory.name.empty())
        output << " $" << memory.name;
    output << " " << memory.initial_block_count;
    if (memory.max_block_count > 0)
        output << " " << memory.max_block_count;
    output << ")";

    if (is_imported(memory))
        output << ")";
    output << "\n";
}

void wat_generator::generate(const wasm::global& global, std::ostream& output, size_t indentation)
{
    const auto ind = indent(indentation);
    output << ind << "(global $" << global.name;
    if (global.access == wasm::access_t::Mut)
    {
        output << " (mut " << repr_wasm_type(global.typ) << ")";
    }
    else
    {
        output << " " << repr_wasm_type(global.typ);
    }
    output << "\n";
    generate(global.init_expression, output, indentation + 2);
    output << ind << ")\n";
}

void wat_generator::generate(const wasm::block& block, std::ostream& output, size_t indentation)
{
    CAPY_ASSERT(block.block_label != nullptr, "Block is missing a branch label");
    const auto ind = indent(indentation);
    output << ind << (block.block_type == wasm::block_type_t::Block ? "block" : "loop");
    if (!block.block_label->name.empty())
        output << " $" << block.block_label->name;
    if (block.return_type != wasm::type_t::None)
    {
        output << " (result " << repr_wasm_type(block.return_type) << ")";
    }
    output << "\n";
    generate(block.instructions, output, indentation + 2);
    output << ind << "end\n";
}

void wat_generator::generate(const wasm::if_block& block, std::ostream& output, size_t indentation)
{
    CAPY_ASSERT(block.block_label != nullptr, "If block is missing a branch label");
    const auto ind = indent(indentation);
    output << ind << "if";
    if (!block.block_label->name.empty())
        output << " $" << block.block_label->name;
    if (block.return_type != wasm::type_t::None)
    {
        output << " (result " << repr_wasm_type(block.return_type) << ")";
    }
    output << "\n";

    generate(block.then_block, output, indentation + 2);
    if (!block.else_block.empty())
    {
        output << ind << "else\n";
        generate(block.else_block, output, indentation + 2);
    }
    output << ind << "end\n";
}

void wat_generator::generate(
    const wasm::instruction_list& expr,
    std::ostream& output,
    size_t indentation
)
{
    for (const auto& instruction : expr)
    {
        CAPY_ASSERT(instruction != nullptr, "Encountered null instruction during WAT generation");
        std::visit(
            [&, this](const auto& value)
            {
                this->generate(value, output, indentation);
            },
            *instruction
        );
    }
}

void wat_generator::generate(
    const wasm::op_no_arg& instruction,
    std::ostream& output,
    size_t indentation
)
{
    output << indent(indentation);
    switch (instruction.op)
    {
        case wasm::operation_t::Nop:
            output << "nop";
            break;
        case wasm::operation_t::Unreachable:
            output << "unreachable";
            break;
        case wasm::operation_t::Drop:
            output << "drop";
            break;
        case wasm::operation_t::Ret:
            output << "return";
            break;
        case wasm::operation_t::MemorySize:
            output << "memory.size";
            break;
        case wasm::operation_t::MemoryGrow:
            output << "memory.grow";
            break;
        case wasm::operation_t::MemoryFill:
            output << "memory.fill";
            break;
        case wasm::operation_t::I64ExtendI32S:
            output << "i64.extend_i32_s";
            break;
        case wasm::operation_t::I64ExtendI32U:
            output << "i64.extend_i32_u";
            break;
        case wasm::operation_t::I32WrapI64:
            output << "i32.wrap_i64";
            break;
        default:
            CAPY_FAIL("Unexpected no-argument operation in WAT generator");
            break;
    }
    output << "\n";
}

void wat_generator::generate(
    const wasm::op_index& instruction,
    std::ostream& output,
    size_t indentation
)
{
    output << indent(indentation);
    switch (instruction.op)
    {
        case wasm::operation_t::LocalGet:
            output << "local.get $";
            break;
        case wasm::operation_t::LocalSet:
            output << "local.set $";
            break;
        case wasm::operation_t::LocalTee:
            output << "local.tee $";
            break;
        case wasm::operation_t::GlobalGet:
            output << "global.get $";
            break;
        case wasm::operation_t::GlobalSet:
            output << "global.set $";
            break;
        case wasm::operation_t::Call:
            output << "call $";
            break;
        default:
            CAPY_FAIL("Unexpected indexed operation in WAT generator");
            break;
    }
    output << instruction.name << "\n";
}

void wat_generator::generate(
    const wasm::op_type& instruction,
    std::ostream& output,
    size_t indentation
)
{
    output << indent(indentation) << repr_wasm_type(instruction.value_type);
    switch (instruction.op)
    {
        case wasm::operation_t::IAdd:
            output << ".add";
            break;
        case wasm::operation_t::ISub:
            output << ".sub";
            break;
        case wasm::operation_t::IMul:
            output << ".mul";
            break;
        case wasm::operation_t::IAnd:
            output << ".and";
            break;
        case wasm::operation_t::IOr:
            output << ".or";
            break;
        case wasm::operation_t::IShl:
            output << ".shl";
            break;
        case wasm::operation_t::Eq:
            output << ".eq";
            break;
        case wasm::operation_t::Ne:
            output << ".ne";
            break;
        case wasm::operation_t::Eqz:
            output << ".eqz";
            break;
        default:
            CAPY_FAIL("Unexpected typed operation in WAT generator");
            break;
    }
    output << "\n";
}

void wat_generator::generate(
    const wasm::op_type_sign& instruction,
    std::ostream& output,
    size_t indentation
)
{
    output << indent(indentation) << repr_wasm_type(instruction.value_type);
    switch (instruction.op)
    {
        case wasm::operation_t::IDiv:
            output << ".div_";
            break;
        case wasm::operation_t::IRem:
            output << ".rem_";
            break;
        case wasm::operation_t::IShr:
            output << ".shr_";
            break;
        case wasm::operation_t::ILt:
            output << ".lt_";
            break;
        case wasm::operation_t::ILte:
            output << ".le_";
            break;
        case wasm::operation_t::IGt:
            output << ".gt_";
            break;
        case wasm::operation_t::IGte:
            output << ".ge_";
            break;
        default:
            CAPY_FAIL("Unexpected signed typed operation in WAT generator");
            break;
    }
    output << sign_wasm_type(instruction.value_type) << "\n";
}

void wat_generator::generate(
    const wasm::op_type_value& instruction,
    std::ostream& output,
    size_t indentation
)
{
    CAPY_ASSERT(instruction.op == wasm::operation_t::Const, "Unexpected typed-value operation in WAT generator");
    output << indent(indentation) << repr_wasm_type(instruction.value_type)
           << ".const " << instruction.value << "\n";
}

void wat_generator::generate(
    const wasm::op_align_offset& instruction,
    std::ostream& output,
    size_t indentation
)
{
    const std::string size = size_wasm_type(instruction.value_type);
    output << indent(indentation) << repr_wasm_type(instruction.value_type);

    if (instruction.op == wasm::operation_t::Load)
    {
        output << ".load";
        if (!size.empty())
            output << size << "_" << sign_wasm_type(instruction.value_type);
    }
    else if (instruction.op == wasm::operation_t::Store)
    {
        output << ".store" << size;
    }
    else
    {
        CAPY_FAIL("Unexpected memory operation in WAT generator");
    }

    if (instruction.offset > 0)
        output << " offset=" << instruction.offset;
    CAPY_ASSERT(instruction.alignment < 32, "Memory alignment exponent is too large");
    output << " align=" << (uint64_t{1} << instruction.alignment) << "\n";
}

void wat_generator::generate(
    const wasm::op_label& instruction,
    std::ostream& output,
    size_t indentation
)
{
    output << indent(indentation);
    if (instruction.op == wasm::operation_t::Br)
        output << "br $";
    else if (instruction.op == wasm::operation_t::BrIf)
        output << "br_if $";
    else
    {
        CAPY_FAIL("Unexpected label operation in WAT generator");
    }
    output << instruction.label.name << "\n";
}

void wat_generator::generate_export(
    const wasm::export_item& export_item,
    std::ostream& output,
    size_t indentation
)
{
    output << indent(indentation) << "(export \"" << export_item.export_name << "\" ("
           << repr_wasm_extern_type(export_item.export_type) << " ";
    if (!export_item.name.empty())
        output << "$" << export_item.name;
    else
        output << export_item.index;
    output << "))\n";
}
