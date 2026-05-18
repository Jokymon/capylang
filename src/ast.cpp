#include "ast.hpp"

void dump_node(std::ostream& os, const context& ctx, const node_expr& root, int indent = 0);

void dump_node(std::ostream& os, const context& ctx, const node_number& n, int indent)
{
    std::string ind = std::string(abs(indent), ' ');

    os << ind << "Num:" << n.number << "\n";
}

void dump_node(std::ostream& os, const context& ctx, const node_char_literal& n, int indent)
{
    std::string ind = std::string(abs(indent), ' ');

    os << ind << "Char literal: \"" << n.ch << "\"\n";
}

void dump_node(std::ostream& os, const context& ctx, const node_string_literal& n, int indent)
{
    std::string ind = std::string(abs(indent), ' ');

    os << ind << "String literal: \"" << n.table_index << "\"\n";
}

void dump_node(std::ostream& os, const context& ctx, const node_bool_literal& n, int indent)
{
    std::string ind = std::string(abs(indent), ' ');

    os << ind << "Bool:" << n.value << "\n";
}

void dump_node(std::ostream& os, const context& ctx, const node_var_reference& n, int indent)
{
    std::string ind = std::string(abs(indent), ' ');

    os << ind << "Var(" << ctx.repr(ctx.symbol_at(n.symbol_ref).symbol_type) << "):" << n.name << "\n";
}

void dump_node(std::ostream& os, const context& ctx, const node_pointer_deref& n, int indent)
{
    std::string ind = std::string(abs(indent), ' ');

    os << ind << "Pointer Deref:\n";
    dump_node(os, ctx, *n.pointer_expression, indent + 4);
}

void dump_node(std::ostream& os, const context& ctx, const node_let_expression& n, int indent)
{
    std::string ind = std::string(abs(indent), ' ');

    os << ind << "Let:\n";
    os << ind << "  " << n.name << "=\n";
    dump_node(os, ctx, *n.init_expression, indent + 4);
}

void dump_node(std::ostream& os, const context& ctx, const node_if_expression& n, int indent)
{
    std::string ind = std::string(abs(indent), ' ');
    os << ind << "If: " << ctx.repr(n.assigned_type) << "\n";
    os << ind << "  Condition:\n";
    dump_node(os, ctx, *n.condition, indent + 4);
    os << ind << "  Then-Body:\n";
    for (const auto& expression : n.then_code)
    {
        dump_node(os, ctx, *expression, indent + 4);
    }
    if (n.else_code.size() > 0)
    {
        os << ind << "  Else-Body:\n";
        for (const auto& expression : n.else_code)
        {
            dump_node(os, ctx, *expression, indent + 4);
        }
    }
}

void dump_node(std::ostream& os, const context& ctx, const node_while_expression& n, int indent)
{
    std::string ind = std::string(abs(indent), ' ');
    os << ind << "While\n";
    os << ind << "  Condition:\n";
    dump_node(os, ctx, *n.condition, indent + 4);
    os << ind << "  While-Body:\n";
    for (const auto& expression : n.while_code)
    {
        dump_node(os, ctx, *expression, indent + 4);
    }
}

void dump_node(std::ostream& os, const context& ctx, const node_record_definition& n, int indent)
{
    std::string ind = std::string(abs(indent), ' ');

    os << ind << "Record definition " << n.name << ":\n";
    for (const auto& field : n.fields)
    {
        os << ind << "  " << field.first << ": " << ctx.repr(field.second) << "\n";
    }
}

void dump_node(std::ostream& os, const context& ctx, const node_record_initialisation& n, int indent)
{
    std::string ind = std::string(abs(indent), ' ');

    os << ind << "Record init\n";
    for (const auto& field_init : n.initialisations)
    {
        os << ind << "  " << field_init.field_name << "=\n";
        dump_node(os, ctx, *field_init.init_expression, indent + 6);
    }
}

void dump_node(std::ostream& os, const context& ctx, const node_field_deref& n, int indent)
{
    std::string ind = std::string(abs(indent), ' ');

    os << ind << "Deref:\n";
    os << ind << "  object:\n";
    dump_node(os, ctx, *n.object, indent + 4);
    os << ind << "  field: " << n.fieldname << "\n";
}

void dump_node(std::ostream& os, const context& ctx, const node_function_head& n, int indent)
{
    std::string ind = std::string(abs(indent), ' ');

    // os << ind << "Function head: TODO\n";
    os << ind << "  Name: " << n.name << "\n";
}

void dump_node(std::ostream& os, const context& ctx, const node_import_definition& n, int indent)
{
    std::string ind = std::string(abs(indent), ' ');

    os << ind << "Import definition: TODO\n";
}

void dump_node(std::ostream& os, const context& ctx, const node_global_definition& n, int indent)
{
    std::string ind = std::string(abs(indent), ' ');

    os << ind << "Global:\n";
    os << ind << "  " << n.name << "=\n";
    // dump_node(os, ctx, *n.init_expression, indent+4);
}

void dump_node(std::ostream& os, const context& ctx, const node_function_call& n, int indent)
{
    std::string ind = std::string(abs(indent), ' ');

    os << ind << "Function call '" << n.function_name << "'\n";
    for (const auto& param : n.parameter)
    {
        os << ind << "  Parameter:\n";
        dump_node(os, ctx, *param, indent + 4);
    }
}

void dump_node(std::ostream& os, const context& ctx, const node_function_definition& n, int indent)
{
    std::string ind = std::string(abs(indent), ' ');

    os << ind << "Function definition:\n";
    dump_node(os, ctx, *n.function_head, indent);
    os << ind << "  Body:\n";
    for (const auto& expression : n.code)
    {
        dump_node(os, ctx, *expression, indent + 4);
    }
}

void dump_node(std::ostream& os, const context& ctx, const node_cast_expression& n, int indent)
{
    std::string ind = std::string(abs(indent), ' ');

    os << ind << "Casting operation"
       << "; target type: " << ctx.repr(n.cast_type) << "\n";

    dump_node(os, ctx, *n.expression, indent + 4);
}

void dump_node(std::ostream& os, const context& ctx, const node_discard_expression& n, int indent)
{
    std::string ind = std::string(abs(indent), ' ');
    os << ind << "Discard:\n";
    dump_node(os, ctx, *n.expression, indent + 4);
}

void dump_node(std::ostream& os, const context& ctx, const node_return_expression& n, int indent)
{
    std::string ind = std::string(abs(indent), ' ');
    os << ind << "Return:\n";
    if (n.expression)
    {
        dump_node(os, ctx, *n.expression, indent + 4);
    }
    else
    {
        os << ind << "  <void>\n";
    }
}

void dump_node(std::ostream& os, const context& ctx, const node_break_statement&, int indent)
{
    std::string ind = std::string(abs(indent), ' ');
    os << ind << "Break\n";
}

void dump_node(std::ostream& os, const context& ctx, const node_unary_expression& n, int indent)
{
    std::string ind = std::string(abs(indent), ' ');

    os << ind << "Unary expression; op: " << repr_op(n.operation)
       << "; type: " << ctx.repr(n.assigned_type) << "\n";

    dump_node(os, ctx, *n.expr, indent + 4);
}

void dump_node(std::ostream& os, const context& ctx, const node_binary_expression& n, int indent)
{
    std::string ind = std::string(abs(indent), ' ');

    os << ind << "Expression; op: " << repr_op(n.operation)
       << "; type: " << ctx.repr(n.assigned_type) << "\n";

    dump_node(os, ctx, *n.left, indent + 4);
    dump_node(os, ctx, *n.right, indent + 4);
}

void dump_node(std::ostream& os, const context& ctx, const node_module& n, int indent)
{
    std::string ind = std::string(abs(indent), ' ');

    os << ind << "Module:\n";
    for (const auto& import : n.imports)
    {
        dump_node(os, ctx, *import, indent + 4);
    }
    for (const auto& type_def : n.typedefs)
    {
        dump_node(os, ctx, *type_def, indent + 4);
    }
    for (const auto& global : n.globals)
    {
        dump_node(os, ctx, *global, indent + 4);
    }
    for (const auto& function : n.functions)
    {
        dump_node(os, ctx, *function, indent + 4);
    }
}

void dump_node(std::ostream& os, const context& ctx, const node_expr& root, int indent)
{
    std::visit(
        [=, &ctx, &os](const auto& n)
        {
            dump_node(os, ctx, n, indent);
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
