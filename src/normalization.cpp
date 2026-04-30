#include "normalization.hpp"

normalization::normalization(context& ctx)
: ast_visitor()
, parse_context(ctx)
{
}

void normalization::normalize(node_module& module)
{
    visit_nodes(module);
}

void normalization::process(source_range, node_number&) {}
void normalization::process(source_range, node_char_literal&) {}
void normalization::process(source_range, node_bool_const&) {}
void normalization::process(source_range, node_string_literal&) {}
void normalization::process(source_range, node_var_reference&) {}
void normalization::process(source_range, node_record_definition&) {}
void normalization::process(source_range, node_field_deref&) {}
void normalization::process(source_range, node_global_definition&) {}
void normalization::process(source_range, node_import_definition&) {}

void normalization::process(source_range, node_pointer_deref& n)
{
    visit_nodes(n);
}

void normalization::process(source_range, node_record_initialisation& n)
{
    visit_nodes(n);
}

void normalization::process(source_range, node_function_call& n)
{
    visit_nodes(n);
}

void normalization::process(source_range, node_cast_expression& n)
{
    visit_nodes(n);
}

void normalization::process(source_range, node_discard_expression& n)
{
    visit_nodes(n);
}

void normalization::process(source_range location, node_negation& n)
{
    visit_nodes(n);
}

void normalization::process(source_range, node_return_expression& n)
{
    visit_nodes(n);
}

void normalization::process(source_range location, node_break_statement&)
{
}

void normalization::process(source_range, node_binary_expression& n)
{
    visit_nodes(n);
}

void normalization::process(source_range, node_if_expression& n)
{
    visit_nodes(n);
}

void normalization::process(source_range, node_while_expression& n)
{
    visit_nodes(n);
}

void normalization::process(source_range, node_let_expression& n)
{
    visit_nodes(n);
}

void normalization::process(source_range, node_function_definition& n)
{
    visit_nodes(n);

    auto maybe_return_type = parse_context.function_return_type(n.function_head->signature.function_type);
    if (!maybe_return_type.has_value())
    {
        return;
    }
    if (parse_context.is_primitive_type(maybe_return_type.value(), primitive_type::Void))
    {
        return;
    }
    if (n.code.empty())
    {
        return;
    }

    if (std::holds_alternative<node_return_expression>(n.code.back()->value) ||
        std::holds_alternative<node_discard_expression>(n.code.back()->value))
    {
        return;
    }

    auto tail = std::move(n.code.back());
    const auto tail_location = tail->location;
    n.code.back() = std::make_unique<node_expr>(node_expr{
        .value = node_return_expression{
            .expression = std::move(tail),
            .is_explicit = false,
        },
        .location = tail_location,
    });
}

void normalization::process(source_range, node_module& n)
{
    visit_nodes(n);
}
