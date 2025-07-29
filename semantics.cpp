#include "semantics.hpp"
#include <iostream>

type_kind assigned_node_type(const ast_node &node)
{
    return std::visit([&](const auto &n)
                      {
        using T = std::decay_t<decltype(n)>;

        if constexpr (std::is_same_v<T, node_number>) {
            return n.assigned_type;
        } else if constexpr (std::is_same_v<T, node_var_reference>) {
            return n.symbol_ref.symbol_type;
        } else if constexpr (std::is_same_v<T, node_function_definition>) {
            return assigned_node_type(*n.function_head);
        } else if constexpr (std::is_same_v<T, node_function_head>) {
            return n.signature.return_type;
        } else if constexpr (std::is_same_v<T, node_function_call>) {
            return n.symbol_ref.signature.return_type;
        } else if constexpr (std::is_same_v<T, node_type_spec>) {
            return n.type_spec;
        } else if constexpr (std::is_same_v<T, node_expression>) {
             return n.assigned_type;
        } else {
            return type_kind::unassigned;
        } }, node.value);
}

std::optional<node_parse_error> semantic_analyser::process(node_number &n)
{
    return std::nullopt;
}

std::optional<node_parse_error> semantic_analyser::process(node_var_reference &n)
{
    return std::nullopt;
}

std::optional<node_parse_error> semantic_analyser::process(node_type_spec &n)
{
    return std::nullopt;
}

std::optional<node_parse_error> semantic_analyser::process(source_range location, node_function_call &n)
{
    function_signature actual_signature;

    for (const auto &param : n.parameter)
    {
        auto res = semantic_analysis(*param);
        if (res.has_value())
        {
            return res;
        }
        actual_signature.parameters.push_back(param_spec{.name = "_", .type_spec = assigned_node_type(*param)});
    }

    if (!actual_signature.equals_call_signature(n.symbol_ref.signature))
    {
        return node_parse_error{
            .error_location = location.start,
            .error_message = "Function '"+n.symbol_ref.name+"' expects signature "
                + n.symbol_ref.signature.repr() + "; called with signature "
                + actual_signature.repr()};
    }

    return std::nullopt;
}

std::optional<node_parse_error> semantic_analyser::process(node_import_definition &n)
{
    return std::nullopt;
}

std::optional<node_parse_error> semantic_analyser::process(node_function_definition &n)
{
    auto declared_return_type = assigned_node_type(*n.function_head);

    auto actual_return_type = type_kind::void_type;
    source_range error_location;
    for (const auto &expression : n.code)
    {
        auto res = semantic_analysis(*expression);
        if (res.has_value())
        {
            return res;
        }
        actual_return_type = assigned_node_type(*expression);
        error_location = expression->location;
    }

    if (declared_return_type != actual_return_type)
    {
        return node_parse_error{
            .error_location = error_location.start,
            .error_message = "Returned value of type "+repr_type(actual_return_type)+
                " doesn't match the declared return type "+repr_type(declared_return_type)};
    }
    return std::nullopt;
}

std::optional<node_parse_error> semantic_analyser::process(source_range location, node_expression &n)
{
    auto lhs_error = semantic_analysis(*n.left);
    if (lhs_error.has_value())
    {
        return lhs_error;
    }
    auto lhs_type = assigned_node_type(*n.left);

    auto rhs_error = semantic_analysis(*n.right);
    if (rhs_error.has_value())
    {
        return rhs_error;
    }
    auto rhs_type = assigned_node_type(*n.right);

    if ((lhs_type == rhs_type) && (lhs_type != type_kind::unassigned))
    {
        n.assigned_type = lhs_type;
        return std::nullopt;
    }
    else if (n.operation == token_operator::op_conversion)
    {
        if (!std::holds_alternative<node_type_spec>(n.right->value))
        {
            return node_parse_error{
                .error_location = location.start,
                .error_message = "Illegal parse tree"};
        }

        n.assigned_type = std::get<node_type_spec>(n.right->value).type_spec;

        return std::nullopt;
    }
    else
    {
        return node_parse_error{
            .error_location = n.op_range.start,
            .error_message = "Types for '" + repr_op(n.operation) +
                             "'-operation do not match; they should be equal but are '" +
                             repr_type(lhs_type) + "' and '" + repr_type(rhs_type) + "'",
        };
    }
}

std::optional<node_parse_error> semantic_analyser::process(node_module &n)
{
    for (const auto &func_def : n.functions)
    {
        auto result = semantic_analysis(*func_def);
        if (result.has_value())
        {
            return result;
        }
    }
    return std::nullopt;
}

std::optional<node_parse_error> semantic_analyser::semantic_analysis(ast_node &root)
{
    return std::visit([&](auto &n) -> std::optional<node_parse_error>
                      {
        using T = std::decay_t<decltype(n)>;

        if constexpr (std::is_same_v<T, node_number>) {
            return process(n);
        } else if constexpr (std::is_same_v<T, node_var_reference>) {
            return process(n);
        } else if constexpr (std::is_same_v<T, node_type_spec>) {
            return process(n);
        } else if constexpr (std::is_same_v<T, node_function_call>) {
            return process(root.location, n);
        } else if constexpr (std::is_same_v<T, node_import_definition>) {
            return process(n);
        } else if constexpr (std::is_same_v<T, node_function_definition>) {
            return process(n);
        } else if constexpr (std::is_same_v<T, node_expression>) {
            return process(root.location, n);
        } else if constexpr (std::is_same_v<T, node_module>) {
            return process(n);
        } else {
            // TODO: This should happen, maybe return special error
            return std::nullopt;
        } }, root.value);
}
