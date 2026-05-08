#include "ast.hpp"
#include <iostream>

void dump_ast(const context& ctx, const node_expr& root, size_t indent = 0);

void dump_node(const context& ctx, const node_number& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');

    std::cout << ind << "Num:" << n.number << "\n";
}

void dump_node(const context& ctx, const node_char_literal& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');

    std::cout << ind << "Char literal: \"" << n.ch << "\"\n";
}

void dump_node(const context& ctx, const node_string_literal& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');

    std::cout << ind << "String literal: \"" << n.table_index << "\"\n";
}

void dump_node(const context& ctx, const node_bool_literal& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');

    std::cout << ind << "Bool:" << n.value << "\n";
}

void dump_node(const context& ctx, const node_var_reference& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');

    std::cout << ind << "Var(" << ctx.repr(ctx.symbol_at(n.symbol_ref).symbol_type) << "):" << n.name << "\n";
}

void dump_node(const context& ctx, const node_pointer_deref& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');

    std::cout << ind << "Pointer Deref:\n";
    dump_ast(ctx, *n.pointer_expression, indent + 4);
}

void dump_node(const context& ctx, const node_let_expression& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');

    std::cout << ind << "Let:\n";
    std::cout << ind << "  " << n.name << "=\n";
    dump_ast(ctx, *n.init_expression, indent + 4);
}

void dump_node(const context& ctx, const node_if_expression& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');
    std::cout << ind << "If: " << ctx.repr(n.assigned_type) << "\n";
    std::cout << ind << "  Condition:\n";
    dump_ast(ctx, *n.condition, indent + 4);
    std::cout << ind << "  Then-Body:\n";
    for (const auto& expression : n.then_code)
    {
        dump_ast(ctx, *expression, indent + 4);
    }
    if (n.else_code.size() > 0)
    {
        std::cout << ind << "  Else-Body:\n";
        for (const auto& expression : n.else_code)
        {
            dump_ast(ctx, *expression, indent + 4);
        }
    }
}

void dump_node(const context& ctx, const node_while_expression& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');
    std::cout << ind << "While\n";
    std::cout << ind << "  Condition:\n";
    dump_ast(ctx, *n.condition, indent + 4);
    std::cout << ind << "  While-Body:\n";
    for (const auto& expression : n.while_code)
    {
        dump_ast(ctx, *expression, indent + 4);
    }
}

void dump_node(const context& ctx, const node_record_definition& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');

    std::cout << ind << "Record definition " << n.name << ":\n";
    for (const auto& field : n.fields)
    {
        std::cout << ind << "  " << field.first << ": " << ctx.repr(field.second) << "\n";
    }
}

void dump_node(const context& ctx, const node_record_initialisation& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');

    std::cout << ind << "Record init\n";
    for (const auto& field_init : n.initialisations)
    {
        std::cout << ind << "  " << field_init.field_name << "=\n";
        dump_ast(ctx, *field_init.init_expression, indent + 6);
    }
}

void dump_node(const context& ctx, const node_field_deref& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');

    std::cout << ind << "Deref:\n";
    std::cout << ind << "  object:\n";
    dump_ast(ctx, *n.object, indent + 4);
    std::cout << ind << "  field: " << n.fieldname << "\n";
}

void dump_node(const context& ctx, const node_function_head& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');

    // std::cout << ind << "Function head: TODO\n";
    std::cout << ind << "  Name: " << n.name << "\n";
}

void dump_node(const context& ctx, const node_import_definition& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');

    std::cout << ind << "Import definition: TODO\n";
}

void dump_node(const context& ctx, const node_global_definition& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');

    std::cout << ind << "Global:\n";
    std::cout << ind << "  " << n.name << "=\n";
    // dump_ast(ctx, *n.init_expression, indent+4);
}

void dump_node(const context& ctx, const node_function_call& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');

    std::cout << ind << "Function call '" << n.function_name << "'\n";
    for (const auto& param : n.parameter)
    {
        std::cout << ind << "  Parameter:\n";
        dump_ast(ctx, *param, indent + 4);
    }
}

void dump_node(const context& ctx, const node_function_definition& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');

    std::cout << ind << "Function definition:\n";
    dump_node(ctx, *n.function_head, indent);
    std::cout << ind << "  Body:\n";
    for (const auto& expression : n.code)
    {
        dump_ast(ctx, *expression, indent + 4);
    }
}

void dump_node(const context& ctx, const node_cast_expression& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');

    std::cout << ind << "Casting operation"
              << "; target type: " << ctx.repr(n.cast_type) << "\n";

    dump_ast(ctx, *n.expression, indent + 4);
}

void dump_node(const context& ctx, const node_discard_expression& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');
    std::cout << ind << "Discard:\n";
    dump_ast(ctx, *n.expression, indent + 4);
}

void dump_node(const context& ctx, const node_return_expression& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');
    std::cout << ind << "Return:\n";
    if (n.expression)
    {
        dump_ast(ctx, *n.expression, indent + 4);
    }
    else
    {
        std::cout << ind << "  <void>\n";
    }
}

void dump_node(const context& ctx, const node_break_statement&, size_t indent)
{
    std::string ind = std::string(indent, ' ');
    std::cout << ind << "Break\n";
}

void dump_node(const context& ctx, const node_unary_expression& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');

    std::cout << ind << "Unary expression; op: " << repr_op(n.operation)
              << "; type: " << ctx.repr(n.assigned_type) << "\n";

    dump_ast(ctx, *n.expr, indent + 4);
}

void dump_node(const context& ctx, const node_binary_expression& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');

    std::cout << ind << "Expression; op: " << repr_op(n.operation)
              << "; type: " << ctx.repr(n.assigned_type) << "\n";

    dump_ast(ctx, *n.left, indent + 4);
    dump_ast(ctx, *n.right, indent + 4);
}

void dump_module(const context& ctx, const node_module& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');

    std::cout << ind << "Module:\n";
    for (const auto& import : n.imports)
    {
        dump_node(ctx, *import, indent + 4);
    }
    for (const auto& type_def : n.typedefs)
    {
        dump_ast(ctx, *type_def, indent + 4);
    }
    for (const auto& global : n.globals)
    {
        dump_node(ctx, *global, indent + 4);
    }
    for (const auto& function : n.functions)
    {
        dump_node(ctx, *function, indent + 4);
    }
}

void dump_ast(const context& ctx, const node_expr& root, size_t indent)
{
    std::visit(
        [=, &ctx](const auto& n)
        {
            dump_node(ctx, n, indent);
        },
        root.value
    );
}

std::string node_field_deref::repr_obj() const
{
    return std::visit(
        [&](const auto& n) -> std::string
        {
            using T = std::decay_t<decltype(n)>;

            if constexpr (std::is_same_v<T, node_field_deref>)
            {
                return n.repr_obj() + "." + n.fieldname;
            }
            else if constexpr (std::is_same_v<T, node_var_reference>)
            {
                return n.name;
            }
            else
            {
                // this branch actually shouldn't happen
                return "";
            }
        },
        object->value
    );
}

bool node_function_definition::has_attribute(const std::string& attr_name) const
{
    for (const auto& attr : attributes)
    {
        if (attr.name == attr_name)
        {
            return true;
        }
    }
    return false;
}

ast_visitor::ast_visitor()
: current_context{assign_context::rhs}
, while_nesting_level(0)
{
}

void ast_visitor::visit(node_expr& root)
{
    std::visit(
        [&](auto& n) -> void
        {
            process(root.location, n);
        },
        root.value
    );
}

void ast_visitor::visit_nodes(node_module& module)
{
    for (const auto& import_def : module.imports)
    {
        process(import_def->location, *import_def);
    }

    for (const auto& global_def : module.globals)
    {
        process(global_def->location, *global_def);
    }

    for (const auto& func_def : module.functions)
    {
        process(func_def->location, *func_def);
    }
}

void ast_visitor::visit_nodes(node_function_definition& func_def)
{
    for (const auto& expression : func_def.code)
    {
        visit(*expression);
    }
}

void ast_visitor::visit_nodes(node_if_expression& i_expr)
{
    visit(*i_expr.condition);

    for (const auto& expression : i_expr.then_code)
    {
        visit(*expression);
    }

    for (const auto& expression : i_expr.else_code)
    {
        visit(*expression);
    }
}

void ast_visitor::visit_nodes(node_while_expression& w_expr)
{
    visit(*w_expr.condition);

    while_nesting_level++;
    for (const auto& expression : w_expr.while_code)
    {
        visit(*expression);
    }
    while_nesting_level--;
}

void ast_visitor::visit_nodes(node_let_expression& l_expr)
{
    if (l_expr.init_expression)
    {
        visit(*l_expr.init_expression);
    }
}

void ast_visitor::visit_nodes(node_unary_expression& expr)
{
    visit(*expr.expr);
}

void ast_visitor::visit_nodes(node_binary_expression& expr)
{
    if (expr.operation == op_assignment)
    {
        current_context = assign_context::lhs;
    }
    else
    {
        current_context = assign_context::rhs;
    }

    visit(*expr.left);
    current_context = assign_context::rhs;
    visit(*expr.right);
}

void ast_visitor::visit_nodes(node_cast_expression& expr)
{
    visit(*expr.expression);
}

void ast_visitor::visit_nodes(node_discard_expression& expr)
{
    visit(*expr.expression);
}

void ast_visitor::visit_nodes(node_return_expression& expr)
{
    if (expr.expression)
    {
        visit(*expr.expression);
    }
}

void ast_visitor::visit_nodes(node_function_call& func_call)
{
    for (const auto& param : func_call.parameter)
    {
        visit(*param);
    }
}

void ast_visitor::visit_nodes(node_record_initialisation& r_init)
{
    for (auto& init : r_init.initialisations)
    {
        visit(*init.init_expression);
    }
}

void ast_visitor::visit_nodes(node_pointer_deref& ptr_deref)
{
    visit(*ptr_deref.pointer_expression);
}
