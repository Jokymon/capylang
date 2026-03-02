#include "ast_visitor.hpp"

ast_visitor::ast_visitor()
: current_context{assign_context::rhs}
{
}

void ast_visitor::visit(ast_node& root)
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

void ast_visitor::visit_nodes(node_import_definition& import_def)
{
    process(source_range{}, *import_def.function_head);
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

    for (const auto& expression : w_expr.while_code)
    {
        visit(*expression);
    }
}

void ast_visitor::visit_nodes(node_let_expression& l_expr)
{
    if (l_expr.init_expression)
    {
        visit(*l_expr.init_expression);
    }
}

void ast_visitor::visit_nodes(node_expression& expr)
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
