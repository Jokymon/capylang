#include "anf.hpp"
#include "semantics.hpp"
#include <iostream>
#include <string>
#include <utility>

std::optional<anf_binary_op> to_anf_binary_op(operator_type op)
{
    switch (op)
    {
        case op_plus:
            return anf_binary_op::add;
        case op_minus:
            return anf_binary_op::sub;
        case op_multiply:
            return anf_binary_op::mul;
        case op_division:
            return anf_binary_op::div;
        case op_modulus:
            return anf_binary_op::mod;
        case op_equals:
            return anf_binary_op::eq;
        case op_notequals:
            return anf_binary_op::ne;
        default:
            return std::nullopt;
    }
}

static std::string indent_str(size_t indent)
{
    return std::string(indent, ' ');
}

static std::string repr_binding(const context& ctx, const anf_binding& b)
{
    if (b.assigned_type == ILLEGAL_TYPE)
    {
        return b.name;
    }
    return b.name + ":" + ctx.repr(b.assigned_type);
}

static std::string repr_binary_op(anf_binary_op op)
{
    switch (op)
    {
        case anf_binary_op::add:
            return "+";
        case anf_binary_op::sub:
            return "-";
        case anf_binary_op::mul:
            return "*";
        case anf_binary_op::div:
            return "/";
        case anf_binary_op::mod:
            return "%";
        case anf_binary_op::eq:
            return "==";
        case anf_binary_op::ne:
            return "!=";
    }

    return "?";
}

static void dump_anf_atom(const context& ctx, const anf_atom& atom, size_t indent)
{
    std::string ind = indent_str(indent);
    std::visit(
        [&](const auto& a)
        {
            using T = std::decay_t<decltype(a)>;
            if constexpr (std::is_same_v<T, anf_atom_var>)
            {
                std::cout << ind << "var " << repr_binding(ctx, a.binding) << "\n";
            }
            else if constexpr (std::is_same_v<T, anf_atom_number>)
            {
                std::cout << ind << "number " << a.value;
                if (a.assigned_type != ILLEGAL_TYPE)
                {
                    std::cout << ":" << ctx.repr(a.assigned_type);
                }
                std::cout << "\n";
            }
            else if constexpr (std::is_same_v<T, anf_atom_bool>)
            {
                std::cout << ind << "bool " << (a.value ? "true" : "false") << "\n";
            }
            else if constexpr (std::is_same_v<T, anf_atom_char>)
            {
                std::cout << ind << "char " << a.ch << "\n";
            }
            else if constexpr (std::is_same_v<T, anf_atom_string>)
            {
                std::cout << ind << "string literal#" << a.table_index << " size=" << a.size << "\n";
            }
            else if constexpr (std::is_same_v<T, anf_atom_pointer_deref>)
            {
                std::cout << ind << "deref\n";
                dump_anf_atom(ctx, anf_atom{a.pointer}, indent + 2);
                if (a.assigned_type != ILLEGAL_TYPE)
                {
                    std::cout << ind << "  type " << ctx.repr(a.assigned_type) << "\n";
                }
            }
        },
        atom
    );
}

static void dump_anf_let_value(const context& ctx, const anf_let_value& value, size_t indent)
{
    std::string ind = indent_str(indent);
    std::visit(
        [&](const auto& v)
        {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, anf_atom>)
            {
                dump_anf_atom(ctx, v, indent);
            }
            else if constexpr (std::is_same_v<T, anf_binary_expression>)
            {
                std::cout << ind << "binary " << repr_binary_op(v.operation) << "\n";
                dump_anf_atom(ctx, v.left, indent + 2);
                dump_anf_atom(ctx, v.right, indent + 2);
            }
            else if constexpr (std::is_same_v<T, anf_call_expression>)
            {
                std::cout << ind << "call " << v.function_name;
                if (v.result_type != ILLEGAL_TYPE)
                {
                    std::cout << " -> " << ctx.repr(v.result_type);
                }
                std::cout << "\n";
                for (const auto& arg : v.arguments)
                {
                    dump_anf_atom(ctx, arg, indent + 2);
                }
            }
            else if constexpr (std::is_same_v<T, anf_record_get_expression>)
            {
                std::cout << ind << "record.get #" << v.field_index << " (" << v.debug_field_name << ")";
                if (v.field_type != ILLEGAL_TYPE)
                {
                    std::cout << " : " << ctx.repr(v.field_type);
                }
                std::cout << "\n";
                dump_anf_atom(ctx, v.object, indent + 2);
            }
        },
        value
    );
}

static void dump_anf_block(const context& ctx, const anf_block& block, size_t indent);

static void dump_anf_statement(const context& ctx, const anf_statement& stmt, size_t indent)
{
    std::string ind = indent_str(indent);
    std::visit(
        [&](const auto& s)
        {
            using T = std::decay_t<decltype(s)>;
            if constexpr (std::is_same_v<T, anf_let_statement>)
            {
                std::cout << ind << "let " << repr_binding(ctx, s.target) << "\n";
                dump_anf_let_value(ctx, s.value, indent + 2);
            }
            else if constexpr (std::is_same_v<T, anf_record_set_statement>)
            {
                std::cout << ind << "record.set #" << s.field_index << " (" << s.debug_field_name << ")\n";
                std::cout << ind << "  object\n";
                dump_anf_atom(ctx, s.object, indent + 4);
                std::cout << ind << "  value\n";
                dump_anf_atom(ctx, s.value, indent + 4);
            }
            else if constexpr (std::is_same_v<T, anf_assign_statement>)
            {
                std::cout << ind << "assign " << repr_binding(ctx, s.target) << "\n";
                dump_anf_atom(ctx, s.value, indent + 2);
            }
            else if constexpr (std::is_same_v<T, anf_call_statement>)
            {
                std::cout << ind << "call " << s.function_name << "\n";
                for (const auto& arg : s.arguments)
                {
                    dump_anf_atom(ctx, arg, indent + 2);
                }
            }
            else if constexpr (std::is_same_v<T, anf_if_statement>)
            {
                std::cout << ind << "if\n";
                std::cout << ind << "  condition\n";
                dump_anf_atom(ctx, s.condition, indent + 4);
                std::cout << ind << "  then\n";
                dump_anf_block(ctx, *s.then_block, indent + 4);
                std::cout << ind << "  else\n";
                dump_anf_block(ctx, *s.else_block, indent + 4);
            }
            else if constexpr (std::is_same_v<T, anf_while_statement>)
            {
                std::cout << ind << "while\n";
                std::cout << ind << "  condition\n";
                dump_anf_atom(ctx, s.condition, indent + 4);
                std::cout << ind << "  body\n";
                dump_anf_block(ctx, *s.body, indent + 4);
            }
        },
        stmt
    );
}

static void dump_anf_block(const context& ctx, const anf_block& block, size_t indent)
{
    if (block.statements.empty())
    {
        std::cout << indent_str(indent) << "<empty>\n";
        return;
    }

    for (const auto& stmt : block.statements)
    {
        dump_anf_statement(ctx, stmt, indent);
    }
}

void dump_anf_module(const context& ctx, const anf_module& module, size_t indent)
{
    std::string ind = indent_str(indent);
    std::cout << ind << "ANF Module:\n";

    if (!module.imports.empty())
    {
        std::cout << ind << "  Imports:\n";
        for (const auto& imp : module.imports)
        {
            std::cout << ind << "    " << imp.ns_name << "::" << imp.function_name;
            if (imp.alias.has_value())
            {
                std::cout << " as " << imp.alias.value();
            }
            std::cout << "\n";
        }
    }

    if (!module.globals.empty())
    {
        std::cout << ind << "  Globals:\n";
        for (const auto& glob : module.globals)
        {
            std::cout << ind << "    " << glob.name;
            if (glob.assigned_type != ILLEGAL_TYPE)
            {
                std::cout << ":" << ctx.repr(glob.assigned_type);
            }
            std::cout << " = " << glob.init_value << "\n";
        }
    }

    std::cout << ind << "  Functions:\n";
    for (const auto& func : module.functions)
    {
        std::cout << ind << "    fn " << repr_binding(ctx, func.function) << "\n";
        dump_anf_block(ctx, *func.body, indent + 6);
    }
}

anf_generator::anf_generator(context& ctx)
: diagnostic_emitter(diagnostic_phase::anf)
, parse_context(ctx)
{
}

anf_module anf_generator::generate(node_module& module)
{
    generated_module = {};
    diagnostics_.clear();
    temp_index = 0;
    lower_module(module);
    return std::move(generated_module);
}

const diagnostic_bag& anf_generator::diagnostics() const
{
    return diagnostics_;
}

diagnostic_bag& anf_generator::diagnostics_sink()
{
    return diagnostics_;
}

void anf_generator::lower_module(const node_module& n)
{
    for (const auto& import_def : n.imports)
    {
        lower_import(*import_def);
    }
    for (const auto& global_def : n.globals)
    {
        lower_global(*global_def);
    }
    for (const auto& function_def : n.functions)
    {
        lower_function(function_def->location, *function_def);
    }
}

void anf_generator::lower_import(const node_import_definition& n)
{
    generated_module.imports.push_back({
        .ns_name = n.ns_name,
        .function_name = n.function_head->name,
        .signature = n.function_head->signature,
        .alias = n.alias,
    });
}

void anf_generator::lower_global(const node_global_definition& n)
{
    generated_module.globals.push_back({
        .name = n.name,
        .symbol_ref = n.symbol_ref,
        .assigned_type = n.assigned_type,
        .init_value = n.init_value,
    });
}

void anf_generator::lower_function(source_range location, const node_function_definition& n)
{
    anf_function_definition func;
    func.attributes = n.attributes;
    func.function = binding_from_symbol(
        n.function_scope->parent->lookup(n.function_head->name).value_or(ILLEGAL_SYMBOL),
        n.function_head->name,
        n.function_head->signature.function_type
    );
    func.signature = n.function_head->signature;
    func.body = std::make_unique<anf_block>();

    lower_block(n.code, *func.body);
    if (func.body->statements.empty() && !n.code.empty())
    {
        append_error_at(
            location.start,
            "function body could not be fully lowered to current ANF subset"
        );
    }

    generated_module.functions.push_back(std::move(func));
}

bool anf_generator::lower_statement(const ast_node& node, anf_block& block)
{
    return std::visit(
        [&](const auto& n) -> bool
        {
            using T = std::decay_t<decltype(n)>;

            if constexpr (std::is_same_v<T, node_let_expression>)
            {
                if (!n.init_expression)
                {
                    return true;
                }

                auto value = lower_let_value(*n.init_expression);
                if (!value.has_value())
                {
                    return false;
                }

                block.statements.push_back(anf_let_statement{
                    .target = binding_from_symbol(n.symbol_ref, n.name, parse_context.symbol_at(n.symbol_ref).symbol_type),
                    .value = std::move(value.value()),
                });
                return true;
            }
            else if constexpr (std::is_same_v<T, node_expression>)
            {
                if (n.operation != op_assignment)
                {
                    return false;
                }

                auto value = lower_atom(*n.right);
                if (!value.has_value())
                {
                    return false;
                }

                if (std::holds_alternative<node_var_reference>(n.left->value))
                {
                    const auto& var_ref = std::get<node_var_reference>(n.left->value);
                    const auto& symbol = parse_context.symbol_at(var_ref.symbol_ref);
                    block.statements.push_back(anf_assign_statement{
                        .target = binding_from_symbol(var_ref.symbol_ref, var_ref.name, symbol.symbol_type),
                        .value = std::move(value.value()),
                    });
                    return true;
                }

                if (!std::holds_alternative<node_field_deref>(n.left->value))
                {
                    return false;
                }

                const auto& field = std::get<node_field_deref>(n.left->value);
                auto obj = lower_atom(*field.object);
                if (!obj.has_value())
                {
                    return false;
                }
                auto field_index = record_field_index(field.object_type, field.fieldname);
                if (!field_index.has_value())
                {
                    append_error_at(
                        node.location.start,
                        "record field '" + field.fieldname + "' could not be resolved to a numeric index"
                    );
                    return false;
                }

                block.statements.push_back(anf_record_set_statement{
                    .object = std::move(obj.value()),
                    .field_index = field_index.value(),
                    .debug_field_name = field.fieldname,
                    .value = std::move(value.value()),
                });
                return true;
            }
            else if constexpr (std::is_same_v<T, node_function_call>)
            {
                const auto& function_symbol = parse_context.symbol_at(n.symbol_ref);
                auto maybe_return_type = parse_context.function_return_type(function_symbol.symbol_type);
                if (!maybe_return_type.has_value())
                {
                    return false;
                }
                if (!parse_context.is_primitive_type(maybe_return_type.value(), primitive_type::Void))
                {
                    return false;
                }

                std::vector<anf_atom> args;
                args.reserve(n.parameter.size());
                for (const auto& p : n.parameter)
                {
                    auto atom = lower_atom(*p);
                    if (!atom.has_value())
                    {
                        return false;
                    }
                    args.push_back(std::move(atom.value()));
                }

                block.statements.push_back(anf_call_statement{
                    .function_name = n.function_name,
                    .function_symbol = n.symbol_ref,
                    .arguments = std::move(args),
                });
                return true;
            }
            else if constexpr (std::is_same_v<T, node_if_expression>)
            {
                auto cond = lower_condition(*n.condition, block);
                if (!cond.has_value())
                {
                    return false;
                }

                auto then_block = std::make_unique<anf_block>();
                lower_block(n.then_code, *then_block);
                auto else_block = std::make_unique<anf_block>();
                lower_block(n.else_code, *else_block);

                block.statements.push_back(anf_if_statement{
                    .condition = std::move(cond.value()),
                    .then_block = std::move(then_block),
                    .else_block = std::move(else_block),
                });
                return true;
            }
            else if constexpr (std::is_same_v<T, node_while_expression>)
            {
                auto cond = lower_condition(*n.condition, block);
                if (!cond.has_value())
                {
                    return false;
                }

                auto body = std::make_unique<anf_block>();
                lower_block(n.while_code, *body);

                block.statements.push_back(anf_while_statement{
                    .condition = std::move(cond.value()),
                    .body = std::move(body),
                });
                return true;
            }
            else
            {
                return false;
            }
        },
        node.value
    );
}

void anf_generator::lower_block(const std::vector<std::unique_ptr<ast_node>>& expressions, anf_block& block)
{
    for (const auto& expression : expressions)
    {
        if (!lower_statement(*expression, block))
        {
            append_error_at(
                expression->location.start,
                "statement is not representable in current ANF subset"
            );
        }
    }
}

std::optional<anf_atom> anf_generator::lower_atom(const ast_node& node) const
{
    return std::visit(
        [&](const auto& n) -> std::optional<anf_atom>
        {
            using T = std::decay_t<decltype(n)>;
            if constexpr (std::is_same_v<T, node_var_reference>)
            {
                const auto& symbol = parse_context.symbol_at(n.symbol_ref);
                return anf_atom_var{
                    .binding = {
                        .name = symbol.name,
                        .symbol_ref = n.symbol_ref,
                        .assigned_type = symbol.symbol_type,
                    }
                };
            }
            else if constexpr (std::is_same_v<T, node_number>)
            {
                return anf_atom_number{
                    .value = n.number,
                    .assigned_type = n.assigned_type,
                };
            }
            else if constexpr (std::is_same_v<T, node_bool_const>)
            {
                return anf_atom_bool{.value = n.value};
            }
            else if constexpr (std::is_same_v<T, node_char_literal>)
            {
                return anf_atom_char{.ch = n.ch};
            }
            else if constexpr (std::is_same_v<T, node_string_literal>)
            {
                return anf_atom_string{
                    .table_index = n.table_index,
                    .size = n.size,
                };
            }
            else if constexpr (std::is_same_v<T, node_pointer_deref>)
            {
                if (n.context != assign_context::rhs)
                {
                    return std::nullopt;
                }
                if (!std::holds_alternative<node_var_reference>(n.pointer_expression->value))
                {
                    return std::nullopt;
                }

                const auto& var_ref = std::get<node_var_reference>(n.pointer_expression->value);
                const auto& symbol = parse_context.symbol_at(var_ref.symbol_ref);

                return anf_atom_pointer_deref{
                    .pointer = anf_atom_var{
                        .binding = {
                            .name = symbol.name,
                            .symbol_ref = var_ref.symbol_ref,
                            .assigned_type = symbol.symbol_type,
                        }
                    },
                    .assigned_type = n.assigned_type,
                };
            }
            else
            {
                return std::nullopt;
            }
        },
        node.value
    );
}

std::optional<anf_let_value> anf_generator::lower_let_value(const ast_node& node)
{
    if (auto atom = lower_atom(node); atom.has_value())
    {
        return anf_let_value{std::move(atom.value())};
    }

    return std::visit(
        [&](const auto& n) -> std::optional<anf_let_value>
        {
            using T = std::decay_t<decltype(n)>;
            if constexpr (std::is_same_v<T, node_expression>)
            {
                auto maybe_op = to_anf_binary_op(n.operation);
                if (!maybe_op.has_value())
                {
                    return std::nullopt;
                }

                auto left = lower_atom(*n.left);
                auto right = lower_atom(*n.right);
                if (!left.has_value() || !right.has_value())
                {
                    return std::nullopt;
                }

                return anf_let_value{
                    anf_binary_expression{
                        .operation = maybe_op.value(),
                        .left = std::move(left.value()),
                        .right = std::move(right.value()),
                    }
                };
            }
            else if constexpr (std::is_same_v<T, node_function_call>)
            {
                std::vector<anf_atom> args;
                args.reserve(n.parameter.size());
                for (const auto& param : n.parameter)
                {
                    auto atom = lower_atom(*param);
                    if (!atom.has_value())
                    {
                        return std::nullopt;
                    }
                    args.push_back(std::move(atom.value()));
                }

                return anf_let_value{
                    anf_call_expression{
                        .function_name = n.function_name,
                        .function_symbol = n.symbol_ref,
                        .arguments = std::move(args),
                        .result_type = assigned_node_type(node, parse_context),
                    }
                };
            }
            else if constexpr (std::is_same_v<T, node_field_deref>)
            {
                auto object = lower_atom(*n.object);
                if (!object.has_value())
                {
                    return std::nullopt;
                }
                auto index = record_field_index(n.object_type, n.fieldname);
                if (!index.has_value())
                {
                    return std::nullopt;
                }

                return anf_let_value{
                    anf_record_get_expression{
                        .object = std::move(object.value()),
                        .field_index = index.value(),
                        .debug_field_name = n.fieldname,
                        .field_type = assigned_node_type(node, parse_context),
                    }
                };
            }
            else
            {
                return std::nullopt;
            }
        },
        node.value
    );
}

std::optional<anf_atom> anf_generator::lower_condition(const ast_node& node, anf_block& block)
{
    if (auto atom = lower_atom(node); atom.has_value())
    {
        return std::move(atom.value());
    }

    auto value = lower_let_value(node);
    if (!value.has_value())
    {
        return std::nullopt;
    }

    anf_binding temp_binding{
        .name = next_temp_name(),
        .symbol_ref = ILLEGAL_SYMBOL,
        .assigned_type = assigned_node_type(node, parse_context),
    };
    anf_atom temp_atom = anf_atom_var{.binding = temp_binding};

    block.statements.push_back(anf_let_statement{
        .target = temp_binding,
        .value = std::move(value.value()),
    });

    return std::move(temp_atom);
}

anf_binding anf_generator::binding_from_symbol(symbol_id symbol_ref, const std::string& fallback_name, type_id explicit_type) const
{
    if (symbol_ref == ILLEGAL_SYMBOL)
    {
        return {
            .name = fallback_name,
            .symbol_ref = ILLEGAL_SYMBOL,
            .assigned_type = explicit_type,
        };
    }

    const auto& symbol = parse_context.symbol_at(symbol_ref);
    return {
        .name = symbol.name,
        .symbol_ref = symbol_ref,
        .assigned_type = (explicit_type == ILLEGAL_TYPE) ? symbol.symbol_type : explicit_type,
    };
}

std::string anf_generator::next_temp_name()
{
    return "_anf_tmp" + std::to_string(temp_index++);
}

std::optional<size_t> anf_generator::record_field_index(type_id record_type_id, const std::string& field_name) const
{
    type_id resolved = parse_context.resolved_type(record_type_id);
    if (!parse_context.is_record_type(resolved))
    {
        return std::nullopt;
    }

    const auto& entry = parse_context.types[resolved];
    const auto& rec = std::get<record_type>(std::get<type_kind>(entry));
    for (size_t i = 0; i < rec.fields.size(); ++i)
    {
        if (rec.fields[i].first == field_name)
        {
            return i;
        }
    }
    return std::nullopt;
}
