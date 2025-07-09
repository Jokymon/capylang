#include "emitter.hpp"

std::string type_mapping(type_kind type_spec)
{
    switch (type_spec)
    {
        case type_kind::s32:
            return "i32";
        case type_kind::u32:
            return "i32";
        default:
            return "";
    }
}

emitter::emitter(std::ostream &output) : output_(output)
{
}

void emitter::generate(const ast_node &node)
{
    output_ << "(module\n";
    output_ << "  (type (;0;) (func))\n";
    output_ << "  (type (;1;) (func (param i32)))\n";
    output_ << "  (import \"wasi_snapshot_preview1\" \"proc_exit\" (func $__imported_wasi_snapshot_preview1_proc_exit (;0;) (type 1)))\n";
    output_ << "  (memory (;0;) 2)\n";
    output_ << "  (export \"memory\" (memory 0))\n";
    output_ << "  (export \"_start\" (func $_start))\n";

    this->emit(node);

    output_ << ")\n";
}

void emitter::emit(const ast_node &node)
{
    std::visit([&, this](const auto &n)
               {
        using T = std::decay_t<decltype(n)>;

        if constexpr (std::is_same_v<T, node_number>) {
            this->emit(n);
        } else if constexpr (std::is_same_v<T, node_function_call>) {
            this->emit(n);
        } else if constexpr (std::is_same_v<T, node_function_definition>) {
            this->emit(n);
        } else if constexpr (std::is_same_v<T, node_expression>) {
            this->emit(n);
        } else if constexpr (std::is_same_v<T, node_module>) {
            this->emit(n);
        } else {} }, node.value);
}

void emitter::emit(const node_module& module_def)
{
    for (const auto& func_def : module_def.functions)
    {
        emit(*func_def);
    }
}

void emitter::emit(const node_function_definition& func_def)
{
    output_ << "  (func " << func_def.function_name;

    for (const auto &param : func_def.parameters)
    {
        output_ << " (param $" << param.name << " " << type_mapping(param.type_spec) << ")";
    }

    if (func_def.return_type != type_kind::void_type)
    {
        output_ << " (result " << type_mapping(func_def.return_type) << ")\n";
    }
    else
    {
        output_ << "\n";
    }
    
    emit(*func_def.code);

    output_ << "  )\n";
}

void emitter::emit(const node_function_call& func_call)
{
    for (const auto &param : func_call.parameter)
    {
        emit(*param);
    }
    output_ << "      call " << func_call.function_name << "\n";
}

void emitter::emit(const node_expression &root)
{
    emit(*root.left);
    emit(*root.right);
    switch (root.operation)
    {
    case token_operator::op_minus:
        output_ << "      i32.sub\n";
        break;
    case token_operator::op_plus:
        output_ << "      i32.add\n";
        break;
    case token_operator::op_multiply:
        output_ << "      i32.mul\n";
        break;
    case token_operator::op_division:
        output_ << "      i32.div_u\n";
        break;
    case token_operator::op_modulus:
        output_ << "      i32.rem_u\n";
        break;
    case token_operator::op_conversion:
        // TODO
        break;
    }
}

void emitter::emit(const node_number &number)
{
    output_ << "      i32.const " << number.number << "\n";
}
