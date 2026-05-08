#include "semantics.hpp"
#include <array>
#include <assert.h>
#include <iostream>
#include <unordered_set>

struct number_range_rule
{
    primitive_type primitive;
    long long min_value;
    long long max_value;
    const char* label;
};

constexpr std::array<number_range_rule, 6> NUMBER_RANGE_RULES{{
    {primitive_type::U8, 0, std::numeric_limits<uint8_t>::max(), "u8"},
    {primitive_type::U16, 0, std::numeric_limits<uint16_t>::max(), "u16"},
    {primitive_type::U32, 0, std::numeric_limits<uint32_t>::max(), "u32"},
    {primitive_type::S8, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::max(), "s8"},
    {primitive_type::S16, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max(), "s16"},
    {primitive_type::S32, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max(), "s32"},
}};

type_id assigned_node_type(const node_expr& node, context& ctx)
{
    return std::visit(
        [&](const auto& n) -> type_id
        {
            using T = std::decay_t<decltype(n)>;

            if constexpr (std::is_same_v<T, node_number>)
            {
                return ctx.resolved_type(n.assigned_type);
            }
            else if constexpr (std::is_same_v<T, node_char_literal>)
            {
                return ctx.intern_primitive(primitive_type::Char);
            }
            else if constexpr (std::is_same_v<T, node_bool_literal>)
            {
                return ctx.intern_primitive(primitive_type::Boolean);
            }
            else if constexpr (std::is_same_v<T, node_string_literal>)
            {
                return ctx.intern_primitive(primitive_type::String);
            }
            else if constexpr (std::is_same_v<T, node_var_reference>)
            {
                return ctx.resolved_type(ctx.symbol_at(n.symbol_ref).symbol_type);
            }
            else if constexpr (std::is_same_v<T, node_pointer_deref>)
            {
                return ctx.resolved_type(n.assigned_type);
            }
            else if constexpr (std::is_same_v<T, node_function_definition>)
            {
                return ctx.function_return_type(n.function_head->signature.function_type).value();
            }
            else if constexpr (std::is_same_v<T, node_function_call>)
            {
                const auto& sym = ctx.symbol_at(n.symbol_ref);
                if (sym.kind != symbol_kind::function)
                {
                    return ILLEGAL_TYPE;
                }
                auto return_type = ctx.function_return_type(sym.signature.function_type);
                if (!return_type.has_value())
                {
                    return ILLEGAL_TYPE;
                }
                return return_type.value();
            }
            else if constexpr (std::is_same_v<T, node_let_expression>)
            {
                return ctx.intern_primitive(primitive_type::Void);
            }
            else if constexpr (std::is_same_v<T, node_record_initialisation>)
            {
                return n.type_spec;
            }
            else if constexpr (std::is_same_v<T, node_if_expression>)
            {
                return ctx.resolved_type(n.assigned_type);
            }
            else if constexpr (std::is_same_v<T, node_while_expression>)
            {
                return ctx.intern_primitive(primitive_type::Void);
            }
            else if constexpr (std::is_same_v<T, node_field_deref>)
            {
                auto field_type = ctx.record_field_type(n.object_type, n.fieldname);
                if (field_type.has_value())
                {
                    return field_type.value();
                }
                return ILLEGAL_TYPE;
            }
            else if constexpr (std::is_same_v<T, node_cast_expression>)
            {
                return n.cast_type;
            }
            else if constexpr (std::is_same_v<T, node_discard_expression>)
            {
                return ctx.intern_primitive(primitive_type::Void);
            }
            else if constexpr (std::is_same_v<T, node_return_expression>)
            {
                return ctx.intern_primitive(primitive_type::Void);
            }
            else if constexpr (std::is_same_v<T, node_unary_expression>)
            {
                return ctx.resolved_type(n.assigned_type);
            }
            else if constexpr (std::is_same_v<T, node_binary_expression>)
            {
                return ctx.resolved_type(n.assigned_type);
            }
            else
            {
                return ILLEGAL_TYPE;
            }
        },
        node.value
    );
}

semantic_analyser::semantic_analyser(diagnostic_bag& diagnostics, context& ctx)
: ast_visitor()
, diagnostic_emitter(diagnostics, diagnostic_phase::semantics)
, parse_context(ctx)
, current_func_head(nullptr)
{
}

void semantic_analyser::semantic_analysis(node_module& module)
{
    visit_nodes(module);
}

void semantic_analyser::process(source_range location, node_number& n)
{
    auto primitive = parse_context.primitive_type_of(n.assigned_type);
    if (primitive.has_value())
    {
        for (const auto& rule : NUMBER_RANGE_RULES)
        {
            if (rule.primitive != primitive.value())
            {
                continue;
            }
            if ((n.number < rule.min_value) || (n.number > rule.max_value))
            {
                append_error_at(
                    location.start,
                    std::format("Numeric literal '{}' exceeds valid range for {} ({}..{})", n.number, rule.label, rule.min_value, rule.max_value)
                );
            }
            break;
        }
    }
}

void semantic_analyser::process(source_range location, node_char_literal& n)
{
}

void semantic_analyser::process(source_range location, node_bool_literal& n)
{
}

void semantic_analyser::process(source_range location, node_string_literal& n)
{
}

void semantic_analyser::process(source_range location, node_var_reference& n)
{
    n.context = current_context;

    auto& sym = parse_context.symbol_at(n.symbol_ref);
    if (sym.kind == symbol_kind::error)
    {
        return;
    }

    if (current_context == assign_context::rhs)
    {
        if (!sym.is_assigned)
        {
            append_error_at(
                location.start,
                "Variable '" + n.name + "' used without assigning a value before"
            );
        }
    }
    else if (current_context == assign_context::lhs)
    {
        sym.is_assigned = true;
    }
}

void semantic_analyser::process(source_range location, node_pointer_deref& n)
{
    n.context = current_context;

    visit_nodes(n);

    type_id expression_type = assigned_node_type(*n.pointer_expression, parse_context);
    if (!parse_context.is_pointer_type(expression_type))
    {
        append_error_at(
            location.start,
            "Can't dereference non-pointer type " + parse_context.repr(expression_type)
        );
    }
    else
    {
        auto* ptr_type = get_type_from_node<pointer_type>(parse_context.types[to_index(expression_type)]);
        assert(ptr_type != nullptr && "Compiler error, in semantics check, the type of a pointer ref should be a pointer");
        n.assigned_type = ptr_type->to;
    }
}

void semantic_analyser::process(source_range location, node_record_definition& n)
{
}

void semantic_analyser::process(source_range location, node_record_initialisation& n)
{
    auto* r = get_type_from_node<record_type>(parse_context.types[to_index(n.type_spec)]);
    assert(r != nullptr && "Compiler error, semantics check should be on a record initialisation");

    std::unordered_set<std::string> expected_fields;
    expected_fields.reserve(r->fields.size());
    for (const auto& field : r->fields)
    {
        expected_fields.insert(field.first);
    }

    std::unordered_set<std::string> initialised_fields;
    initialised_fields.reserve(n.initialisations.size());
    for (const auto& init : n.initialisations)
    {
        initialised_fields.insert(init.field_name);
    }

    for (const auto& field : r->fields)
    {
        if (!initialised_fields.contains(field.first))
        {
            append_error_at(
                location.start,
                "Record field '" + field.first + "' not initialised"
            );
        }
    }

    visit_nodes(n);

    for (const auto& init : n.initialisations)
    {
        if (!expected_fields.contains(init.field_name))
        {
            append_error_at(
                init.location,
                "Unknown record field '" + init.field_name + "'"
            );
        }
    }
}

void semantic_analyser::process(source_range location, node_field_deref& n)
{
    if (!parse_context.is_record_type(n.object_type))
    {
        append_error_at(
            location.start,
            "Dereferencing non-record variable or field '" + n.repr_obj() + "'"
        );
        return;
    }

    if (parse_context.is_primitive_type(n.object_type, primitive_type::String))
    {
        if ((n.fieldname != "ptr") && (n.fieldname != "size"))
        {
            append_error_at(
                location.start,
                "Unknown field '" + n.fieldname + "' for string type"
            );
        }
    }
    else
    {
        auto field_type = parse_context.record_field_type(n.object_type, n.fieldname);
        if (!field_type.has_value())
        {
            append_error_at(
                location.start,
                "Unknown record field '" + n.fieldname + "'"
            );
        }
    }
}

void semantic_analyser::process(source_range location, node_function_call& n)
{
    const auto& symbol = parse_context.symbol_at(n.symbol_ref);
    if (symbol.kind != symbol_kind::function)
    {
        return;
    }

    visit_nodes(n);

    function_type actual_func_type;
    for (const auto& param : n.parameter)
    {
        actual_func_type.parameter_types.push_back(
            assigned_node_type(*param, parse_context)
        );
    }

    auto declared_type = parse_context.types[to_index(symbol.signature.function_type)];
    auto* declared_function_type = get_type_from_node<function_type>(declared_type);

    if (!actual_func_type.is_call_signature_eq(*declared_function_type))
    {
        append_error_at(
            location.start,
            "Function '" + symbol.name + "' expects signature " + declared_function_type->repr_call_sig(parse_context) + "; called with signature " + actual_func_type.repr_call_sig(parse_context)
        );
    }
}

void semantic_analyser::process(source_range location, node_if_expression& n)
{
    visit_nodes(n);

    type_id then_return_type = parse_context.intern_primitive(primitive_type::Void);
    if (!n.then_code.empty())
    {
        then_return_type = assigned_node_type(*n.then_code.back(), parse_context);
    }

    type_id else_return_type = parse_context.intern_primitive(primitive_type::Void);
    if (!n.else_code.empty())
    {
        else_return_type = assigned_node_type(*n.else_code.back(), parse_context);
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
                    "' and '" + parse_context.repr(else_return_type) + "'"
            );
        }
    }

    n.assigned_type = then_return_type;
}

void semantic_analyser::process(source_range location, node_while_expression& n)
{
    visit_nodes(n);
}

void semantic_analyser::process(source_range location, node_let_expression& n)
{
    if (!n.init_expression)
    {
        // There is no init expression to run any semantic analysis on, so we
        // can skip this here
        return;
    }

    visit_nodes(n);

    const auto& symbol = parse_context.symbol_at(n.symbol_ref);
    if (parse_context.resolved_type(symbol.symbol_type) !=
        assigned_node_type(*n.init_expression, parse_context))
    {
        append_error_at(
            location.start,
            "Type mismatch in let statement. Variable is of type '" + parse_context.repr(symbol.symbol_type) + "' but expression has type '" + parse_context.repr(assigned_node_type(*n.init_expression, parse_context)) + "'"
        );
    }
}

void semantic_analyser::process(source_range location, node_import_definition& n)
{
}

void semantic_analyser::process(source_range location, node_global_definition& n)
{
}

void semantic_analyser::process(source_range location, node_function_definition& n)
{
    auto declared_return_type = parse_context.function_return_type(n.function_head->signature.function_type);
    if (!declared_return_type.has_value())
    {
        assert(false && "In a node_function_definition, the function_type should be a function and have a return type");
        append_error_at(
            location.start,
            "Fatal parser error in semantics check from this source location"
        );
        return;
    }

    current_func_head = n.function_head.get();
    visit_nodes(n);
    current_func_head = nullptr;

    type_id actual_return_type = parse_context.intern_primitive(primitive_type::Void);
    source_range error_location;
    if (!n.code.empty())
    {
        actual_return_type = assigned_node_type(*n.code.back(), parse_context);
        error_location = n.code.back()->location;
    }

    if (declared_return_type.value() != actual_return_type)
    {
        append_error_at(
            error_location.start,
            "Returned value of type " + parse_context.repr(actual_return_type) +
                " doesn't match the declared return type " +
                parse_context.repr(declared_return_type.value())
        );
    }
}

void semantic_analyser::process(source_range location, node_cast_expression& n)
{
    visit_nodes(n);
}

void semantic_analyser::process(source_range location, node_discard_expression& n)
{
    visit_nodes(n);
}

void semantic_analyser::process(source_range location, node_return_expression& n)
{
    visit_nodes(n);

    source_position error_position = location.end;
    type_id return_expression_type = parse_context.intern_primitive(primitive_type::Void);
    if (n.expression)
    {
        return_expression_type = assigned_node_type(*n.expression, parse_context);
        error_position = n.expression->location.start;
    }

    auto declared_return_type = parse_context.function_return_type(current_func_head->signature.function_type);

    if (declared_return_type.value() != return_expression_type)
    {
        append_error_at(
            error_position,
            "Returned expression has type '" + parse_context.repr(return_expression_type) + "' but function expects '" +
                parse_context.repr(declared_return_type.value()) + "'"
        );
    }
}

void semantic_analyser::process(source_range location, node_break_statement&)
{
    if (while_nesting_level == 0)
    {
        append_error_at(
            location.start,
            "'break' is not allowed outside 'while' loops"
        );
    }
}

void semantic_analyser::process(source_range location, node_unary_expression& n)
{
    visit_nodes(n);
    n.assigned_type = assigned_node_type(*n.expr, parse_context);
}

void semantic_analyser::process(source_range location, node_binary_expression& n)
{
    visit_nodes(n);

    auto lhs_type = assigned_node_type(*n.left, parse_context);
    auto rhs_type = assigned_node_type(*n.right, parse_context);

    // propagate the type upwards based on the operands
    if (n.operation == op_assignment)
    {
        n.assigned_type = parse_context.intern_primitive(primitive_type::Void);
        if (std::holds_alternative<node_pointer_deref>(n.left->value) ||
            std::holds_alternative<node_var_reference>(n.left->value))
        {
            auto* var_node = std::get_if<node_var_reference>(&n.left->value);
            if (var_node != nullptr)
            {
                const auto& symbol = parse_context.symbol_at(var_node->symbol_ref);
                // TODO: What about mutable pointers?
                if (!symbol.mutab)
                {
                    append_error_at(
                        location.start,
                        "Can't assign to immutable variable '" + symbol.name + "'"
                    );
                }
            }
        }
        else
        {
            append_error_at(
                location.start,
                "Trying to assign to non-lvalue expression"
            );
        }
    }
    else if ((n.operation == op_equals) || (n.operation == op_notequals) || (n.operation == op_lessthan) || (n.operation == op_lessthan_equal) || (n.operation == op_greaterthan) || (n.operation == op_greaterthan_equal))
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

void semantic_analyser::process(source_range location, node_module& module)
{
    visit_nodes(module);
}
