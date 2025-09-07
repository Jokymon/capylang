#include "emitter.hpp"
#include "semantics.hpp"

std::string create_wasm_name(const std::string capy_name)
{
    return "$" + capy_name;
}

std::string type_mapping(type_kind type_spec)
{
    return std::visit([&](const auto &t) -> std::string {
        using T = std::decay_t<decltype(t)>;

        if constexpr (std::is_same_v<T, t_t::s32>) {
            return "i32";
        } else if constexpr (std::is_same_v<T, t_t::u32>) {
            return "i32";
        } else {
            return "";
        }
    }, type_spec);
}

emitter::emitter(std::ostream &output) : output_(output)
{
}

void emitter::generate(const ast_node &node)
{
    this->emit(node);
}

void emitter::emit(const ast_node &node)
{
    std::visit([&, this](const auto &n)
               {
        using T = std::decay_t<decltype(n)>;

        if constexpr (std::is_same_v<T, node_number>) {
            this->emit(n);
        } else if constexpr (std::is_same_v<T, node_var_reference>) {
            this->emit(n);
        } else if constexpr (std::is_same_v<T, node_function_call>) {
            this->emit(n);
        } else if constexpr (std::is_same_v<T, node_import_definition>) {
            this->emit(n);
        } else if constexpr (std::is_same_v<T, node_function_definition>) {
            this->emit(n);
        } else if constexpr (std::is_same_v<T, node_function_head>) {
            this->emit(n);
        } else if constexpr (std::is_same_v<T, node_let_expression>) {
            this->emit(n);
        } else if constexpr (std::is_same_v<T, node_expression>) {
            this->emit(n);
        } else if constexpr (std::is_same_v<T, node_module>) {
            this->emit(n);
        } else {} }, node.value);
}

void emitter::emit(const node_module &module_def)
{
    output_ << "(module\n";

    for (const auto &import_def : module_def.imports)
    {
        emit(*import_def);
    }

    output_ << "  (memory (;0;) 2)\n";
    output_ << "  (data (i32.const 100) \"\\42\\10Test\")\n";
    output_ << "  (export \"memory\" (memory 0))\n";
    output_ << "  (export \"_start\" (func $_start))\n";

    for (const auto &func_def : module_def.functions)
    {
        emit(*func_def);
    }

    output_ << ")\n";
}

void emitter::emit(const node_import_definition &import_def)
{
    output_ << "  (import \"" << import_def.ns_name << "\" ";
    // TODO: this is really ugly, but unfortunately, the "plain" name is only needed
    // in the import definition of WAT
    output_ << "\"" << std::get<node_function_head>(import_def.function_head->value).name << "\" ";

    emit(*import_def.function_head);
    output_ << "))\n";
}

void emitter::emit(const node_function_head &function_head)
{
    emit_function_signature(function_head.name, function_head.signature);
}

void emitter::emit(const node_function_definition &func_def)
{
    output_ << "  ";
    emit(*func_def.function_head);
    output_ << "\n";

    for (auto& [identifier, symbol]: func_def.function_scope->symbol_table)
    {
        if (symbol.kind == symbol_kind::local_var)
        {
            output_ << "      (local " << create_wasm_name(identifier) << " i32)\n";
        }
    }

    for (const auto &expression : func_def.code)
    {
        emit(*expression);
    }

    output_ << "  )\n";
}

void emitter::emit(const node_function_call &func_call)
{
    for (const auto &param : func_call.parameter)
    {
        emit(*param);
    }
    output_ << "      call " << create_wasm_name(func_call.function_name) << "\n";
}

void emitter::emit(const node_let_expression& let_expression)
{
    emit(*let_expression.init_expression);
    output_ << "      local.set " << let_expression.symbol_ref.index_addr << "\n";
}

void emitter::emit(const node_expression &root)
{
    emit(*root.left);
    emit(*root.right);
    switch (root.operation)
    {
    case op_minus:
        output_ << "      i32.sub\n";
        break;
    case op_plus:
        output_ << "      i32.add\n";
        break;
    case op_multiply:
        output_ << "      i32.mul\n";
        break;
    case op_division:
        output_ << "      i32.div_u\n";
        break;
    case op_modulus:
        output_ << "      i32.rem_u\n";
        break;
    case op_conversion:
        if ((t_t::is_of<t_t::void_type>(assigned_node_type(*root.right))) &&
            (!t_t::is_of<t_t::void_type>(assigned_node_type(*root.left))))
        {
            output_ << "      drop\n";
        }
        break;
    }
}

void emitter::emit(const node_number &number)
{
    output_ << "      i32.const " << number.number << "\n";
}

void emitter::emit(const node_var_reference &variable)
{
    output_ << "      local.get " << variable.symbol_ref.index_addr << "\n";
}

void emitter::emit_function_signature(const std::string &function_name, const function_signature &signature)
{
    output_ << "(func " << create_wasm_name(function_name);

    for (const auto &param : signature.parameters)
    {
        output_ << " (param $" << param.name << " " << type_mapping(param.type_spec) << ")";
    }

    if (!t_t::is_of<t_t::void_type>(signature.return_type))
    {
        output_ << " (result " << type_mapping(signature.return_type) << ")";
    }
}
