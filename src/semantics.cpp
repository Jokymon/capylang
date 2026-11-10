#include "semantics.hpp"
#include "number_literal.hpp"
#include "tools.hpp"
#include <array>
#include <cstdint>
#include <limits>
#include <unordered_set>

std::string repr_field_deref_object(const ast::expr& object)
{
    return std::visit(
        [](const auto& node) -> std::string
        {
            using node_type = std::decay_t<decltype(node)>;
            if constexpr (std::is_same_v<node_type, ast::var_reference>)
            {
                return node.name;
            }
            else if constexpr (std::is_same_v<node_type, ast::field_deref>)
            {
                return repr_field_deref_object(*node.object) + "." + node.fieldname;
            }
            else
            {
                return {};
            }
        },
        object
    );
}

struct number_range_rule
{
    primitive_type primitive;
    std::int64_t min_value;
    std::uint64_t max_value;
    const char* label;
};

constexpr std::array<number_range_rule, 8> NUMBER_RANGE_RULES{{
    {primitive_type::U8, 0, std::numeric_limits<uint8_t>::max(), "u8"},
    {primitive_type::U16, 0, std::numeric_limits<uint16_t>::max(), "u16"},
    {primitive_type::U32, 0, std::numeric_limits<uint32_t>::max(), "u32"},
    {primitive_type::U64, 0, std::numeric_limits<uint64_t>::max(), "u64"},
    {primitive_type::S8, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::max(), "s8"},
    {primitive_type::S16, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max(), "s16"},
    {primitive_type::S32, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max(), "s32"},
    {primitive_type::S64, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max(), "s64"},
}};

type_id assigned_node_type(const ast::expr& node, context& ctx)
{
    return std::visit(
        [&](const auto& n) -> type_id
        {
            using T = std::decay_t<decltype(n)>;

            if constexpr (std::is_same_v<T, ast::number>)
            {
                return ctx.resolved_type(n.assigned_type);
            }
            else if constexpr (std::is_same_v<T, ast::char_literal>)
            {
                return ctx.intern_primitive(primitive_type::Char);
            }
            else if constexpr (std::is_same_v<T, ast::bool_literal>)
            {
                return ctx.intern_primitive(primitive_type::Boolean);
            }
            else if constexpr (std::is_same_v<T, ast::string_literal>)
            {
                return ctx.BUILTIN_STRING;
            }
            else if constexpr (std::is_same_v<T, ast::var_reference>)
            {
                return ctx.resolved_type(ctx.symbol_at(n.symbol_ref).symbol_type);
            }
            else if constexpr (std::is_same_v<T, ast::pointer_deref>)
            {
                return ctx.resolved_type(n.assigned_type);
            }
            else if constexpr (std::is_same_v<T, ast::function_definition>)
            {
                return ctx.function_return_type(n.function_head->signature.function_type).value();
            }
            else if constexpr (std::is_same_v<T, ast::function_call>)
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
            else if constexpr (std::is_same_v<T, ast::let_expression>)
            {
                return ctx.intern_primitive(primitive_type::Void);
            }
            else if constexpr (std::is_same_v<T, ast::record_initialisation>)
            {
                return n.type_spec;
            }
            else if constexpr (std::is_same_v<T, ast::if_expression>)
            {
                return ctx.resolved_type(n.assigned_type);
            }
            else if constexpr (std::is_same_v<T, ast::while_expression>)
            {
                return ctx.intern_primitive(primitive_type::Void);
            }
            else if constexpr (std::is_same_v<T, ast::field_deref>)
            {
                auto rec_type = ctx.record_behind(n.object_type);
                CAPY_ASSERT(rec_type.has_value(), "Base types for field derefs must either be records or pointers");
                type_id obj_type = rec_type.value();

                auto field_entry = ctx.record_field_entry(obj_type, n.fieldname);
                if (field_entry.has_value())
                {
                    return field_entry.value().type;
                }
                return ILLEGAL_TYPE;
            }
            else if constexpr (std::is_same_v<T, ast::cast_expression>)
            {
                return n.cast_type;
            }
            else if constexpr (std::is_same_v<T, ast::discard_expression>)
            {
                return ctx.intern_primitive(primitive_type::Void);
            }
            else if constexpr (std::is_same_v<T, ast::return_expression>)
            {
                return ctx.intern_primitive(primitive_type::Void);
            }
            else if constexpr (std::is_same_v<T, ast::unary_expression>)
            {
                return ctx.resolved_type(n.assigned_type);
            }
            else if constexpr (std::is_same_v<T, ast::binary_expression>)
            {
                return ctx.resolved_type(n.assigned_type);
            }
            else
            {
                return ILLEGAL_TYPE;
            }
        },
        node
    );
}

semantic_analyser::semantic_analyser(diagnostic_bag& diagnostics, context& ctx)
: ast::visitor()
, diagnostic_emitter(diagnostics, diagnostic_phase::semantics)
, parse_context(ctx)
, current_context{ast::assign_context_t::Rhs}
, while_nesting_level(0)
, current_func_head(nullptr)
{
}

void semantic_analyser::semantic_analysis(ast::module& module)
{
    visit(module);
}

void semantic_analyser::leave(ast::number& n)
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
            if (!literal_fits(n, rule.min_value, rule.max_value))
            {
                append_error_at(
                    n.location.start,
                    std::format("Numeric literal '{}' exceeds valid range for {} ({}..{})", literal_repr(n), rule.label, rule.min_value, rule.max_value)
                );
            }
            break;
        }
    }
}

void semantic_analyser::leave(ast::var_reference& n)
{
    n.context = current_context;

    auto& sym = parse_context.symbol_at(n.symbol_ref);
    if (sym.kind == symbol_kind::error)
    {
        return;
    }

    if (current_context == ast::assign_context_t::Rhs)
    {
        if (!sym.is_assigned)
        {
            append_error_at(
                n.location.start,
                "Variable '" + n.name + "' used without assigning a value before"
            );
        }
    }
    else if (current_context == ast::assign_context_t::Lhs)
    {
        sym.is_assigned = true;
    }
}

void semantic_analyser::leave(ast::pointer_deref& n)
{
    type_id expression_type = assigned_node_type(*n.pointer_expression, parse_context);
    if (!parse_context.is_pointer_type(expression_type))
    {
        append_error_at(
            n.location.start,
            "Can't dereference non-pointer type " + parse_context.repr(expression_type)
        );
    }
    else
    {
        auto* ptr_type = get_type_from_node<pointer_type>(parse_context.types[to_index(expression_type)]);
        CAPY_ASSERT(ptr_type != nullptr, "Compiler error, in semantics check, the type of a pointer ref should be a pointer");
        n.assigned_type = ptr_type->to;
    }
}

void semantic_analyser::leave(ast::record_initialisation& n)
{
    auto rec_type = parse_context.record_behind(n.type_spec);
    CAPY_ASSERT(rec_type.has_value(), "Compiler error, semantics check should be on a record initialisation");
    auto* r = get_type_from_node<record_type>(parse_context.types[to_index(rec_type.value())]);
    CAPY_ASSERT(r != nullptr, "Compiler error, semantics check should be on a record initialisation");

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
            // TODO: Add the name of the type in the error output
            append_error_at(
                n.location.start,
                "Record field '" + field.first + "' not initialised"
            );
        }
    }

    for (const auto& init : n.initialisations)
    {
        if (!expected_fields.contains(init.field_name))
        {
            append_error_at(
                init.location.start,
                "Unknown record field '" + init.field_name + "'"
            );
        }
    }
}

void semantic_analyser::leave(ast::field_deref& n)
{
    n.object_type = assigned_node_type(*n.object, parse_context);

    auto rec_type = parse_context.record_behind(n.object_type);
    if (!rec_type.has_value() || !parse_context.is_record_type(rec_type.value()))
    {
        append_error_at(
            n.location.start,
            "Dereferencing non-record variable or field '" + repr_field_deref_object(*n.object) + "'"
        );
        return;
    }

    type_id obj_typ = rec_type.value();
    auto field_entry = parse_context.record_field_entry(obj_typ, n.fieldname);

    if (!field_entry.has_value())
    {
        append_error_at(
            n.location.start,
            "Unknown record field '" + n.fieldname + "'"
        );
    }
}

void semantic_analyser::leave(ast::function_call& n)
{
    const auto& symbol = parse_context.symbol_at(n.symbol_ref);
    if (symbol.kind != symbol_kind::function)
    {
        return;
    }

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
            n.location.start,
            "Function '" + symbol.name + "' expects signature " + declared_function_type->repr_call_sig(parse_context) + "; called with signature " + actual_func_type.repr_call_sig(parse_context)
        );
    }
}

void semantic_analyser::leave(ast::if_expression& n)
{
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
                n.location.start,
                "'if' with return type is missing an 'else' branch"
            );
        }
        else
        {
            append_error_at(
                n.location.start,
                "'then' and 'else' branches have mismatching types '" +
                    parse_context.repr(then_return_type) +
                    "' and '" + parse_context.repr(else_return_type) + "'"
            );
        }
    }

    n.assigned_type = then_return_type;
}

void semantic_analyser::enter(ast::while_expression& n)
{
    // TODO: This will already raise the while-nesting-level for the
    // condition expression; This shouldn't really be a problem since
    // the nesting level is only relevant for break statements and we
    // don't want any break statements in the condition
    while_nesting_level++;
}

void semantic_analyser::leave(ast::while_expression& n)
{
    while_nesting_level--;
}

void semantic_analyser::leave(ast::let_expression& n)
{
    if (!n.init_expression)
    {
        // There is no init expression to run any semantic analysis on, so we
        // can skip this here
        return;
    }

    auto init_type = assigned_node_type(*n.init_expression, parse_context);
    if (parse_context.is_primitive_type(init_type, primitive_type::Void))
    {
        append_error_at(
            n.location.start,
            "Cannot initialize a variable with an expression that returns no value"
        );
        return;
    }

    const auto& symbol = parse_context.symbol_at(n.symbol_ref);
    if (parse_context.resolved_type(symbol.symbol_type) != init_type)
    {
        append_error_at(
            n.location.start,
            "Type mismatch in let statement. Variable is of type '" + parse_context.repr(symbol.symbol_type) + "' but expression has type '" + parse_context.repr(init_type) + "'"
        );
    }
}

void semantic_analyser::enter(ast::function_definition& n)
{
    current_func_head = &n.function_head;
}

void semantic_analyser::leave(ast::function_definition& n)
{
    current_func_head = nullptr;

    auto declared_return_type = parse_context.function_return_type(n.function_head.signature.function_type);
    if (!declared_return_type.has_value())
    {
        CAPY_FAIL("In a ast::function_definition, the function_type should be a function and have a return type");
        append_error_at(
            n.location.start,
            "Fatal parser error in semantics check from this source location"
        );
        return;
    }

    type_id actual_return_type = parse_context.intern_primitive(primitive_type::Void);
    source_range error_location;
    if (!n.code.empty())
    {
        actual_return_type = assigned_node_type(*n.code.back(), parse_context);
        error_location = ast::as_trait<ast::location>(*n.code.back()).location;
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

void semantic_analyser::leave(ast::return_expression& n)
{
    source_position error_position = n.location.end;
    type_id return_expression_type = parse_context.intern_primitive(primitive_type::Void);
    if (n.expression)
    {
        return_expression_type = assigned_node_type(*n.expression, parse_context);
        error_position = ast::as_trait<ast::location>(*n.expression).location.start;
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

void semantic_analyser::leave(ast::break_statement& n)
{
    if (while_nesting_level == 0)
    {
        append_error_at(
            n.location.start,
            "'break' is not allowed outside 'while' loops"
        );
    }
}

void semantic_analyser::leave(ast::unary_expression& n)
{
    n.assigned_type = assigned_node_type(*n.expr, parse_context);
}

void semantic_analyser::enter(ast::binary_expression& n)
{
    if (n.operation == op_assignment)
    {
        current_context = ast::assign_context_t::Lhs;
    }
    else
    {
        current_context = ast::assign_context_t::Rhs;
    }
    // TODO: we have to be able to reset the assign_context to Rhs after
    // visiting the left operand
}

void semantic_analyser::leave(ast::binary_expression& n)
{
    auto lhs_type = assigned_node_type(*n.left, parse_context);
    auto rhs_type = assigned_node_type(*n.right, parse_context);

    // propagate the type upwards based on the operands
    if (n.operation == op_assignment)
    {
        n.assigned_type = parse_context.intern_primitive(primitive_type::Void);
        if (std::holds_alternative<ast::pointer_deref>(*n.left) ||
            std::holds_alternative<ast::var_reference>(*n.left) ||
            std::holds_alternative<ast::field_deref>(*n.left))
        {
            auto* var_node = std::get_if<ast::var_reference>(&*n.left);
            if (var_node != nullptr)
            {
                const auto& symbol = parse_context.symbol_at(var_node->symbol_ref);
                // TODO: What about mutable pointers and records?
                if (!symbol.mutab)
                {
                    append_error_at(
                        n.location.start,
                        "Can't assign to immutable variable '" + symbol.name + "'"
                    );
                }
            }
        }
        else
        {
            append_error_at(
                n.location.start,
                "Trying to assign to non-lvalue expression"
            );
        }
    }
    else if ((n.operation == op_equals) || (n.operation == op_notequals) || (n.operation == op_lessthan) || (n.operation == op_lessthan_equal) || (n.operation == op_greaterthan) || (n.operation == op_greaterthan_equal))
    {
        n.assigned_type = parse_context.intern_primitive(primitive_type::Boolean);
        // Both operands must have the same resolved type.
        auto resolved_lhs = parse_context.resolved_type(lhs_type);
        auto resolved_rhs = parse_context.resolved_type(rhs_type);
        if (parse_context.is_resolved(resolved_lhs) && parse_context.is_resolved(resolved_rhs) &&
            (resolved_lhs != resolved_rhs))
        {
            append_error_at(
                n.op_range.start,
                "Types for '" + repr_op(n.operation) +
                    "'-operation do not match; they should be equal but are '" +
                    parse_context.repr(resolved_lhs) + "' and '" + parse_context.repr(resolved_rhs) + "'"
            );
        }
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
