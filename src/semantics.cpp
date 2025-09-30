#include "semantics.hpp"
#include <iostream>

std::optional<type_kind> record_field_type(const type_kind& record, const std::string& field_name)
{
    if (!std::holds_alternative<t_t::record>(record))
    {
        return std::nullopt;
    }

    const t_t::record& r = std::get<t_t::record>(record);

    for (const auto& field_definition : r.fields) {
        if (field_definition.name == field_name)
        {
            return *field_definition.type_spec;
        }
    }

    // we didn't find a field with the given name yet, so it doesn't exist in this definition
    return std::nullopt;
}

type_kind assigned_node_type(const ast_node &node)
{
    return std::visit([&](const auto &n) -> type_kind
                      {
        using T = std::decay_t<decltype(n)>;

        if constexpr (std::is_same_v<T, node_number>) {
            return n.assigned_type;
        } else if constexpr (std::is_same_v<T, node_var_reference>) {
            return n.symbol_ref.symbol_type;
        } else if constexpr (std::is_same_v<T, node_pointer_deref>) {
            return n.assigned_type;
        } else if constexpr (std::is_same_v<T, node_function_definition>) {
            return assigned_node_type(*n.function_head);
        } else if constexpr (std::is_same_v<T, node_function_head>) {
            return n.signature.return_type;
        } else if constexpr (std::is_same_v<T, node_function_call>) {
            return n.symbol_ref.signature.return_type;
        } else if constexpr (std::is_same_v<T, node_let_expression>) {
            return t_t::void_type{};
        } else if constexpr (std::is_same_v<T, node_type_spec>) {
            return n.type_spec;
        } else if constexpr (std::is_same_v<T, node_field_deref>) {
            auto field_type = record_field_type(n.object_type, n.fieldname);
            if (field_type.has_value())
            {
                return field_type.value();
            }
            return t_t::unassigned{};
        } else if constexpr (std::is_same_v<T, node_expression>) {
             return n.assigned_type;
        } else {
            return t_t::unassigned{};
        } }, node.value);
}

semantic_analyser::semantic_analyser()
    : current_context{assign_context::rhs}
{
}

std::optional<node_parse_error> semantic_analyser::process(node_number &n)
{
    return std::nullopt;
}

std::optional<node_parse_error> semantic_analyser::process(node_var_reference &n)
{
    n.context = current_context;
    return std::nullopt;
}

std::optional<node_parse_error> semantic_analyser::process(node_pointer_deref &n)
{
    n.context = current_context;

    auto res = semantic_analysis(*n.pointer_expression);
    if (res.has_value())
    {
        return res;
    }

    type_kind expression_type = assigned_node_type(*n.pointer_expression);
    if (!std::holds_alternative<t_t::pointer>(expression_type))
    {
        return node_parse_error{
            .error_location = source_position{0, 0},
            .error_message = "Can't dereference non-pointer type " + repr_type(expression_type)};
    }
    n.assigned_type = *std::get<t_t::pointer>(expression_type).base_type;
    return std::nullopt;
}

std::optional<node_parse_error> semantic_analyser::process(node_type_spec &n)
{
    return std::nullopt;
}

std::optional<node_parse_error> semantic_analyser::process(source_range location, node_record_initialisation &n)
{
    t_t::record& r = std::get<t_t::record>(n.type_spec);
    for (const auto& field : r.fields)
    {
        bool is_initialised = false;
        for (const auto& init : n.initialisations)
        {
            if (field.name == init.field_name)
            {
                is_initialised = true;
                break;
            }
        }
        if (!is_initialised)
        {
            return node_parse_error{
                .error_location = location.start,
                .error_message = "Record field '"+field.name+"' not initialised"
            };
        }
    }

    for (const auto& init : n.initialisations)
    {
        bool is_actual_field = false;
        for (const auto& field : r.fields)
        {
            if (field.name == init.field_name)
            {
                is_actual_field = true;
                break;
            }
        }
        if (!is_actual_field)
        {
            return node_parse_error{
                .error_location = init.location,
                .error_message = "Unknown record field '"+init.field_name+"'"
            };
        }
    }

    return std::nullopt;
}

std::optional<node_parse_error> semantic_analyser::process(source_range location, node_field_deref &n)
{
    if (!std::holds_alternative<t_t::record>(n.object_type))
    {
        return node_parse_error{
            .error_location = location.start,
            .error_message = "Dereferencing non-record variable or field '"+n.repr_obj()+"'"
        };
    }
    auto field_type = record_field_type(n.object_type, n.fieldname);

    if (!field_type.has_value())
    {
        return node_parse_error{
            .error_location = location.start,
            .error_message = "Unknown record field '"+n.fieldname+"'"};
    }
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
            .error_message = "Function '" + n.symbol_ref.name + "' expects signature "
                + n.symbol_ref.signature.repr() + "; called with signature "
                + actual_signature.repr()};
    }

    return std::nullopt;
}

std::optional<node_parse_error> semantic_analyser::process(node_let_expression &n)
{
    return semantic_analysis(*n.init_expression);
}

std::optional<node_parse_error> semantic_analyser::process(node_import_definition &n)
{
    return std::nullopt;
}

std::optional<node_parse_error> semantic_analyser::process(node_function_definition &n)
{
    auto declared_return_type = assigned_node_type(*n.function_head);

    type_kind actual_return_type = t_t::void_type{};
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
            .error_message = "Returned value of type " + repr_type(actual_return_type) +
                             " doesn't match the declared return type " + repr_type(declared_return_type)};
    }
    return std::nullopt;
}

std::optional<node_parse_error> semantic_analyser::process(source_range location, node_expression &n)
{
    if (n.operation == op_assignment) {
        current_context = assign_context::lhs;
    }
    else {
        current_context = assign_context::rhs;
    }

    auto lhs_error = semantic_analysis(*n.left);
    if (lhs_error.has_value())
    {
        return lhs_error;
    }
    auto lhs_type = assigned_node_type(*n.left);

    current_context = assign_context::rhs;
    auto rhs_error = semantic_analysis(*n.right);
    if (rhs_error.has_value())
    {
        return rhs_error;
    }
    auto rhs_type = assigned_node_type(*n.right);

    // propagate the type upwards based on the operands
    if (n.operation == op_assignment)
    {
        if (std::holds_alternative<node_pointer_deref>(n.left->value) ||
            std::holds_alternative<node_var_reference>(n.left->value))
        {
            n.assigned_type = t_t::void_type{};
            return std::nullopt;
        }
        else
        {
            return node_parse_error{
                .error_location = location.start,
                .error_message = "Trying to assign to non-lvalue expression"};
        }
    }
    else if ((lhs_type == rhs_type) && (!t_t::is_of<t_t::unassigned>(lhs_type)))
    {
        n.assigned_type = lhs_type;
        return std::nullopt;
    }
    else if (n.operation == op_conversion)
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
        } else if constexpr (std::is_same_v<T, node_pointer_deref>) {
            return process(n);
        } else if constexpr (std::is_same_v<T, node_field_deref>) {
            return process(root.location, n);
        } else if constexpr (std::is_same_v<T, node_record_initialisation>) {
            return process(root.location, n);
        } else if constexpr (std::is_same_v<T, node_type_spec>) {
            return process(n);
        } else if constexpr (std::is_same_v<T, node_function_call>) {
            return process(root.location, n);
        } else if constexpr (std::is_same_v<T, node_let_expression>) {
            return process(n);
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
