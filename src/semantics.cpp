#include "semantics.hpp"
#include <iostream>

std::optional<type_kind> record_field_type(const type_kind& record, const std::string& field_name)
{
    if (std::holds_alternative<t_t::record>(record))
    {
        const t_t::record& r = std::get<t_t::record>(record);

        for (const auto& field_definition : r.fields) {
            if (field_definition.name == field_name)
            {
                return *field_definition.type_spec;
            }
        }
    }
    else if (std::holds_alternative<t_t::string>(record))
    {
        if (field_name == "size")
        {
            return t_t::u32{};
        }
        else if (field_name == "ptr")
        {
            return t_t::pointer{t_t::u8{}};
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
        } else if constexpr (std::is_same_v<T, node_char_literal>) {
            return t_t::char_type{};
        } else if constexpr (std::is_same_v<T, node_bool_const>) {
            return t_t::boolean{};
        } else if constexpr (std::is_same_v<T, node_string_literal>) {
            return t_t::string{};
        } else if constexpr (std::is_same_v<T, node_var_reference>) {
            return n.symbol_ref.get().symbol_type;
        } else if constexpr (std::is_same_v<T, node_pointer_deref>) {
            return n.assigned_type;
        } else if constexpr (std::is_same_v<T, node_function_definition>) {
            return n.function_head->signature.return_type;
        } else if constexpr (std::is_same_v<T, node_function_call>) {
            return n.symbol_ref.signature.return_type;
        } else if constexpr (std::is_same_v<T, node_let_expression>) {
            return t_t::void_type{};
        } else if constexpr (std::is_same_v<T, node_record_initialisation>) {
            return n.type_spec;
        } else if constexpr (std::is_same_v<T, node_if_expression>) {
            return n.assigned_type;
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

void semantic_analyser::append_error_at(source_position location, const std::string &error_message)
{
    errors.emplace_back(parse_error(
        location,
        error_message));
}

void semantic_analyser::semantic_analysis(node_module &module)
{
    for (const auto &func_def : module.functions)
    {
        visit(*func_def);
    }
}

void semantic_analyser::process(node_number &n)
{
}

void semantic_analyser::process(source_range location, node_var_reference &n)
{
    if (current_context==assign_context::rhs)
    {
        if (!n.symbol_ref.get().is_assigned)
        {
            append_error_at(
                location.start,
                "Variable '" + n.name + "' used without assigning a value before"
            );
        }
    }
    else if (current_context==assign_context::lhs)
    {
        n.symbol_ref.get().is_assigned = true;
    }
    n.context = current_context;
}

void semantic_analyser::process(node_pointer_deref &n)
{
    n.context = current_context;

    visit(*n.pointer_expression);

    type_kind expression_type = assigned_node_type(*n.pointer_expression);
    if (!std::holds_alternative<t_t::pointer>(expression_type))
    {
        append_error_at(
            source_position{0, 0},
            "Can't dereference non-pointer type " + repr_type(expression_type)
        );
    }
    n.assigned_type = *std::get<t_t::pointer>(expression_type).base_type;
}

void semantic_analyser::process(node_type_spec &n)
{
}

void semantic_analyser::process(source_range location, node_record_initialisation &n)
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
            append_error_at(
                location.start,
                "Record field '"+field.name+"' not initialised"
            );
        }
    }

    for (const auto& init : n.initialisations)
    {
        visit(*init.init_expression);

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
            append_error_at(
                init.location,
                "Unknown record field '"+init.field_name+"'"
            );
        }
    }
}

void semantic_analyser::process(source_range location, node_field_deref &n)
{
    if (!t_t::is_record_like(n.object_type))
    {
        append_error_at(
            location.start,
            "Dereferencing non-record variable or field '"+n.repr_obj()+"'"
        );
        // TODO: if we already know that its not a record type, then
        // we can also skip the check for a valid field name
    }

    if (t_t::is_of<t_t::string>(n.object_type))
    {
        if ((n.fieldname!="ptr") && (n.fieldname!="size"))
        {
            append_error_at(
                location.start,
                "Unknown field '"+n.fieldname+"' for string type");
        }
    }
    else
    {
        auto field_type = record_field_type(n.object_type, n.fieldname);
        if (!field_type.has_value())
        {
            append_error_at(
                location.start,
                "Unknown record field '"+n.fieldname+"'");
        }
    }
}

void semantic_analyser::process(source_range location, node_function_call &n)
{
    function_signature actual_signature;

    for (const auto &param : n.parameter)
    {
        visit(*param);
        actual_signature.parameters.push_back(param_spec{.name = "_", .type_spec = assigned_node_type(*param)});
    }

    if (!actual_signature.equals_call_signature(n.symbol_ref.signature))
    {
        append_error_at(
            location.start,
            "Function '" + n.symbol_ref.name + "' expects signature "
                + n.symbol_ref.signature.repr() + "; called with signature "
                + actual_signature.repr());
    }
}

void semantic_analyser::process(source_range location, node_if_expression &n)
{
    visit(*n.condition);

    type_kind then_return_type = t_t::void_type{};
    for (const auto &expression : n.then_code)
    {
        visit(*expression);
        then_return_type = assigned_node_type(*expression);
    }

    type_kind else_return_type = t_t::void_type{};
    for (const auto &expression : n.else_code)
    {
        visit(*expression);
        else_return_type = assigned_node_type(*expression);
    }

    if (then_return_type != else_return_type)
    {
        if (n.else_code.empty())
        {
            append_error_at(
                location.start,
                "'if' with return type is missing an 'else' branch"
            );
        }
        else
        {
            append_error_at(
                location.start,
                "'then' and 'else' branches have mismatching types '" + repr_type(then_return_type) +
                    "' and '" + repr_type(else_return_type) + "'");
        }
    }

    n.assigned_type = then_return_type;
}

void semantic_analyser::process(source_range location, node_let_expression &n)
{
    if (!n.init_expression)
    {
        // There is no init expression to run any semantic analysis on, so we
        // can skip this here
        return;
        // TODO: we still need to add some bigger scope checks that a variable
        // will indeed get assigned to eventually and that the types do match
    }

    visit(*n.init_expression);

    if (n.assigned_type!=assigned_node_type(*n.init_expression))
    {
        append_error_at(
            location.start,
            "Type mismatch in let statement. Variable is of type '"
                + repr_type(n.assigned_type) + "' but expression has type '"
                + repr_type(assigned_node_type(*n.init_expression))+"'"
        );
    }
}

void semantic_analyser::process(node_import_definition &n)
{
}

void semantic_analyser::process(node_function_definition &n)
{
    auto declared_return_type = n.function_head->signature.return_type;

    type_kind actual_return_type = t_t::void_type{};
    source_range error_location;
    for (const auto &expression : n.code)
    {
        visit(*expression);
        actual_return_type = assigned_node_type(*expression);
        error_location = expression->location;
    }

    if (declared_return_type != actual_return_type)
    {
        append_error_at(
            error_location.start,
            "Returned value of type " + repr_type(actual_return_type) +
                " doesn't match the declared return type " + repr_type(declared_return_type));
    }
}

void semantic_analyser::process(source_range location, node_expression &n)
{
    if (n.operation == op_assignment) {
        current_context = assign_context::lhs;
    }
    else {
        current_context = assign_context::rhs;
    }

    visit(*n.left);
    auto lhs_type = assigned_node_type(*n.left);

    current_context = assign_context::rhs;
    visit(*n.right);
    auto rhs_type = assigned_node_type(*n.right);

    // propagate the type upwards based on the operands
    if (n.operation == op_assignment)
    {
        if (std::holds_alternative<node_pointer_deref>(n.left->value) ||
            std::holds_alternative<node_var_reference>(n.left->value))
        {
            n.assigned_type = t_t::void_type{};
            auto* var_node = std::get_if<node_var_reference>(&n.left->value);
            if (var_node!=nullptr)
            {
                // TODO: What about mutable pointers?
                if (!var_node->symbol_ref.get().mutab)
                {
                    append_error_at(
                        location.start,
                        "Can't assign to immutable variable '" + var_node->symbol_ref.get().name + "'");
                }
            }
        }
        else
        {
            append_error_at(
                location.start,
                "Trying to assign to non-lvalue expression");
        }
    }
    else if ((lhs_type == rhs_type) && (!t_t::is_of<t_t::unassigned>(lhs_type)))
    {
        n.assigned_type = lhs_type;
    }
    else if (n.operation == op_conversion)
    {
        if (!std::holds_alternative<node_type_spec>(n.right->value))
        {
            append_error_at(
                location.start,
                "Illegal parse tree");
        }

        n.assigned_type = std::get<node_type_spec>(n.right->value).type_spec;
    }
    else
    {
        append_error_at(
            n.op_range.start,
            "Types for '" + repr_op(n.operation) +
                "'-operation do not match; they should be equal but are '" +
                repr_type(lhs_type) + "' and '" + repr_type(rhs_type) + "'"
        );
    }
}

void semantic_analyser::visit(ast_node &root)
{
    std::visit([&](auto &n) -> void
                      {
        using T = std::decay_t<decltype(n)>;

        if constexpr (std::is_same_v<T, node_number>) {
            process(n);
        } else if constexpr (std::is_same_v<T, node_var_reference>) {
            process(root.location, n);
        } else if constexpr (std::is_same_v<T, node_pointer_deref>) {
            process(n);
        } else if constexpr (std::is_same_v<T, node_field_deref>) {
            process(root.location, n);
        } else if constexpr (std::is_same_v<T, node_record_initialisation>) {
            process(root.location, n);
        } else if constexpr (std::is_same_v<T, node_type_spec>) {
            process(n);
        } else if constexpr (std::is_same_v<T, node_function_call>) {
            process(root.location, n);
        } else if constexpr (std::is_same_v<T, node_if_expression>) {
            process(root.location, n);
        } else if constexpr (std::is_same_v<T, node_let_expression>) {
            process(root.location, n);
        } else if constexpr (std::is_same_v<T, node_import_definition>) {
            process(n);
        } else if constexpr (std::is_same_v<T, node_function_definition>) {
            process(n);
        } else if constexpr (std::is_same_v<T, node_expression>) {
            process(root.location, n);
        } else {
            // TODO: This should happen, maybe return special error
        } }, root.value);
}
