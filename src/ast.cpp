#include "ast.hpp"

// val_repr() functions are used in DUMPER_CTX_DUMP_FUNC to dispatch on
// the context-based type to dump
std::string val_repr(const context& ctx, type_id tid)
{
    return ctx.repr(tid);
}

std::string val_repr(const context& ctx, symbol_id sid)
{
    // TODO: find a better suitable repr for a symbol entry
    return ctx.repr(ctx.symbol_at(sid).symbol_type);
}

// plain streaming functions for dumping some special scalar types
inline std::ostream& operator<<(std::ostream& o, assign_context a_ctx)
{
    switch (a_ctx)
    {
        case assign_context::lhs:
            o << "lhs";
            break;
        case assign_context::rhs:
            o << "rhs";
            break;
    }
    return o;
}

inline std::ostream& operator<<(std::ostream& o, operator_type op)
{
    o << "\"" << repr_op(op) << "\"";
    return o;
}

inline std::ostream& operator<<(std::ostream& o, function_signature sig)
{
    // TODO
    return o;
}

bool located_node_with_attributes::has_attribute(const std::string& attr_name) const
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

#define DEFINE_DUMP_FUNCS
#define DUMPER_CTX_TYPE const context&
#define DUMPER_CTX_DUMP_FUNC(cntxt, val) val_repr(cntxt, val)
#include "dumpable.hpp"
#include "ast_nodes.hpp"
#undef DEFINE_DUMP_FUNCS

void dump_node(std::ostream& os, const context& ctx, const node_expr& node, int indent)
{
    std::visit(
        [indent, &ctx, &os](auto&& arg)
        { dump_node(os, ctx, arg, indent); },
        node.value
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
