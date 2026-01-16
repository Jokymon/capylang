#include "semantics.hpp"
#include <assert.h>
#include <iostream>

type_id assigned_node_type(const ast_node &node, context& ctx)
{
    return std::visit([&](const auto &n) -> type_id {
        using T = std::decay_t<decltype(n)>;

        if constexpr (std::is_same_v<T, node_number>) {
            return n.assigned_type;
        } else if constexpr (std::is_same_v<T, node_char_literal>) {
            return ctx.intern_primitive(primitive_type::Char);
        } else if constexpr (std::is_same_v<T, node_bool_const>) {
            return ctx.intern_primitive(primitive_type::Boolean);
        } else if constexpr (std::is_same_v<T, node_string_literal>) {
            return ctx.intern_primitive(primitive_type::String);
        } else if constexpr (std::is_same_v<T, node_var_reference>) {
            return n.symbol_ref.get().symbol_type;
        } else if constexpr (std::is_same_v<T, node_pointer_deref>) {
            return n.assigned_type;
        } else if constexpr (std::is_same_v<T, node_function_definition>) {
            return ctx.function_return_type(n.function_head->signature.function_type).value();
        } else if constexpr (std::is_same_v<T, node_function_call>) {
            return ctx.function_return_type(n.symbol_ref.get().signature.function_type).value();
        } else if constexpr (std::is_same_v<T, node_let_expression>) {
            return ctx.intern_primitive(primitive_type::Void);
        } else if constexpr (std::is_same_v<T, node_record_initialisation>) {
            return n.type_spec;
        } else if constexpr (std::is_same_v<T, node_if_expression>) {
            return n.assigned_type;
        } else if constexpr (std::is_same_v<T, node_while_expression>) {
            return ctx.intern_primitive(primitive_type::Void);
        } else if constexpr (std::is_same_v<T, node_field_deref>) {
            auto field_type = ctx.record_field_type(n.object_type, n.fieldname);
            if (field_type.has_value())
            {
                return field_type.value();
            }
            return ILLEGAL_TYPE;
        } else if constexpr (std::is_same_v<T, node_cast_expression>) {
             return n.cast_type;
        } else if constexpr (std::is_same_v<T, node_expression>) {
             return n.assigned_type;
        } else {
            return ILLEGAL_TYPE;
        } }, node.value);
}

semantic_analyser::semantic_analyser(context& ctx)
: current_context{assign_context::rhs}
, parse_context(ctx)
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

void semantic_analyser::process(source_range location, node_number &n)
{
}

void semantic_analyser::process(source_range location, node_char_literal &n)
{
}

void semantic_analyser::process(source_range location, node_bool_const &n)
{
}

void semantic_analyser::process(source_range location, node_string_literal &n)
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

void semantic_analyser::process(source_range location, node_pointer_deref &n)
{
    n.context = current_context;

    visit(*n.pointer_expression);

    type_id expression_type = assigned_node_type(*n.pointer_expression, parse_context);
    if (!parse_context.is_pointer_type(expression_type))
    {
        append_error_at(
            source_position{"", 0, 0},
            "Can't dereference non-pointer type " + parse_context.repr(expression_type)
        );
    }
    else
    {
        auto t = std::get<type_kind>(parse_context.types[expression_type]);
        auto ptr_type = std::get<pointer_type>(t);
        n.assigned_type = ptr_type.to;
    }
}

void semantic_analyser::process(source_range location, node_record_definition &n)
{
}

void semantic_analyser::process(source_range location, node_record_initialisation &n)
{
    auto t = std::get<type_kind>(parse_context.types[n.type_spec]);
    const record_type& r = std::get<record_type>(t);
    for (const auto& field : r.fields)
    {
        bool is_initialised = false;
        for (const auto& init : n.initialisations)
        {
            if (field.first == init.field_name)
            {
                is_initialised = true;
                break;
            }
        }
        if (!is_initialised)
        {
            append_error_at(
                location.start,
                "Record field '"+field.first+"' not initialised"
            );
        }
    }

    for (const auto& init : n.initialisations)
    {
        visit(*init.init_expression);

        bool is_actual_field = false;
        for (const auto& field : r.fields)
        {
            if (field.first == init.field_name)
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
    if (!parse_context.is_record_type(n.object_type))
    {
        append_error_at(
            location.start,
            "Dereferencing non-record variable or field '"+n.repr_obj()+"'"
        );
        // TODO: if we already know that its not a record type, then
        // we can also skip the check for a valid field name
    }

    if (parse_context.is_primitive_type(n.object_type, primitive_type::String))
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
        auto field_type = parse_context.record_field_type(n.object_type, n.fieldname);
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
    function_type actual_func_type;
    for (const auto &param : n.parameter)
    {
        visit(*param);
        actual_func_type.parameter_types.push_back(
            assigned_node_type(*param, parse_context));
    }

    auto declared_type = parse_context.types[n.symbol_ref.get().signature.function_type];
    auto declared_function_type = std::get<function_type>(std::get<type_kind>(declared_type));

    if (!actual_func_type.is_call_signature_eq(declared_function_type))
    {
        append_error_at(
            location.start,
            "Function '" + n.symbol_ref.get().name + "' expects signature "
                + declared_function_type.repr_call_sig(parse_context)
                + "; called with signature "
                + actual_func_type.repr_call_sig(parse_context));
    }
}

void semantic_analyser::process(source_range location, node_if_expression &n)
{
    visit(*n.condition);

    type_id then_return_type = parse_context.intern_primitive(primitive_type::Void);
    for (const auto &expression : n.then_code)
    {
        visit(*expression);
        then_return_type = assigned_node_type(*expression, parse_context);
    }

    type_id else_return_type = parse_context.intern_primitive(primitive_type::Void);
    for (const auto &expression : n.else_code)
    {
        visit(*expression);
        else_return_type = assigned_node_type(*expression, parse_context);
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
                "'then' and 'else' branches have mismatching types '" + 
                    parse_context.repr(then_return_type) +
                    "' and '" + parse_context.repr(else_return_type) + "'");
        }
    }

    n.assigned_type = then_return_type;
}

void semantic_analyser::process(source_range location, node_while_expression &n)
{
    visit(*n.condition);

    for (const auto &expression : n.while_code)
    {
        visit(*expression);
    }
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

    if (n.symbol_ref.get().symbol_type !=
        assigned_node_type(*n.init_expression, parse_context))
    {
        append_error_at(
            location.start,
            "Type mismatch in let statement. Variable is of type '"
                + parse_context.repr(n.symbol_ref.get().symbol_type) + "' but expression has type '"
                + parse_context.repr(assigned_node_type(*n.init_expression, parse_context))+"'"
        );
    }
}

void semantic_analyser::process(source_range location, node_import_definition &n)
{
}

void semantic_analyser::process(source_range location, node_global &n)
{
}

void semantic_analyser::process(source_range location, node_function_definition &n)
{
    auto declared_return_type = parse_context.function_return_type(n.function_head->signature.function_type);
    if (!declared_return_type.has_value())
    {
        assert(false || "In a node_function_definition, the function_type should be a function and have a return type");
        append_error_at(location.start,
            "Fatal parser error in semantics check from this source location");
        return;
    }

    type_id actual_return_type = parse_context.intern_primitive(primitive_type::Void);
    source_range error_location;
    for (const auto &expression : n.code)
    {
        visit(*expression);

        // TODO: move these parts out of the loop, we already know which entry
        // to check for the actual return type
        actual_return_type = assigned_node_type(*expression, parse_context);
        error_location = expression->location;
    }

    if (declared_return_type.value() != actual_return_type)
    {
        append_error_at(
            error_location.start,
            "Returned value of type " + parse_context.repr(actual_return_type) +
                " doesn't match the declared return type " + 
                parse_context.repr(declared_return_type.value()));
    }
}

void semantic_analyser::process(source_range location, node_cast_expression &n)
{
    visit(*n.expression);
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
    auto lhs_type = assigned_node_type(*n.left, parse_context);

    current_context = assign_context::rhs;
    visit(*n.right);
    auto rhs_type = assigned_node_type(*n.right, parse_context);

    // propagate the type upwards based on the operands
    if (n.operation == op_assignment)
    {
        n.assigned_type = parse_context.intern_primitive(primitive_type::Void);
        if (std::holds_alternative<node_pointer_deref>(n.left->value) ||
            std::holds_alternative<node_var_reference>(n.left->value))
        {
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
    else if ((n.operation == op_equals) || (n.operation == op_notequals))
    {
        n.assigned_type = parse_context.intern_primitive(primitive_type::Boolean);
    }
    else if ((lhs_type == rhs_type) && (!parse_context.is_type_var(lhs_type)))
    {
        n.assigned_type = lhs_type;
    }
    else
    {
        n.assigned_type = ILLEGAL_TYPE;
        append_error_at(
            n.op_range.start,
            "Types for '" + repr_op(n.operation) +
                "'-operation do not match; they should be equal but are '" +
                parse_context.repr(lhs_type) + "' and '" + parse_context.repr(rhs_type) + "'"
        );
    }
}

void semantic_analyser::visit(ast_node &root)
{
    std::visit([&](auto &n) -> void {
        process(root.location, n);
    }, root.value);
}
