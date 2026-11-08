#include "type_inference.hpp"
#include "semantics.hpp"
#include <ranges>

type_inference::type_inference(diagnostic_bag& diagnostics, context& ctx)
: diagnostic_emitter(diagnostics, diagnostic_phase::type_inference)
, parse_context(ctx)
{
}

void type_inference::infer_types(ast::module& module)
{
    visit(module);

    unify();
    default_unresolved_integer_types();
    unify();
}

void type_inference::default_unresolved_integer_types()
{
    const auto default_type = parse_context.intern_primitive(primitive_type::S32);
    for (const auto& constraint : parse_context.constraints)
    {
        // Only integer literals without a contextual type receive a default.
        // Other unresolved types must still be inferred or diagnosed.
        if (const auto* numeric = std::get_if<numeric_constraint>(&constraint);
            numeric != nullptr && !parse_context.is_resolved(numeric->n))
        {
            parse_context.resolve(numeric->n, default_type);
        }
    }
}

void type_inference::unify()
{
    // Repeat until no constraint resolves another type variable. Resolving one
    // variable can make further constraints resolvable (e.g. chains like
    // 'let a = 5; let b = a; b == 7u64').
    // TODO: Ensure all types are resolved after inference and report any
    // remaining unresolved types.
    bool progress = true;
    while (progress)
    {
        progress = false;
        for (const auto& constraint : parse_context.constraints)
        {
            std::visit(
                [&](const auto& constr)
                {
                    using T = std::decay_t<decltype(constr)>;

                    if constexpr (std::is_same_v<T, equal_constraint>)
                    {
                        if (parse_context.resolve(constr.a, constr.b))
                        {
                            progress = true;
                        }
                    }
                },
                constraint
            );
        }
    }
}

void type_inference::leave(ast::function_call& n)
{
    const auto& called_symbol = parse_context.symbol_at(n.symbol_ref);
    if (called_symbol.kind != symbol_kind::function)
    {
        return;
    }

    auto declared_type = parse_context.types[to_index(called_symbol.signature.function_type)];
    auto* declared_function_type = get_type_from_node<function_type>(declared_type);

    for (auto [actual_value, declared_param] : std::views::zip(n.parameter, declared_function_type->parameter_types))
    {
        parse_context.constraints.emplace_back(equal_constraint{assigned_node_type(*actual_value, parse_context), declared_param});
    }
}

void type_inference::leave(ast::binary_expression& n)
{
    switch (n.operation)
    {
        case operator_type::op_plus:
        case operator_type::op_minus:
        case operator_type::op_division:
        case operator_type::op_multiply:
        case operator_type::op_modulus:
        // the shift amount must have the same width as the shifted value
        case operator_type::op_shl:
        case operator_type::op_shr:
            // Arithmetic results share the operand type. This also lets a
            // typed result propagate its type back to both operands.
            parse_context.constraints.emplace_back(
                equal_constraint{
                    n.assigned_type,
                    assigned_node_type(*n.left, parse_context)
                }
            );
            [[fallthrough]];
        // both sides of comparisons must have the same type
        case operator_type::op_equals:
        case operator_type::op_notequals:
        case operator_type::op_lessthan:
        case operator_type::op_lessthan_equal:
        case operator_type::op_greaterthan:
        case operator_type::op_greaterthan_equal:
        // the assigned value gets the type of the target (e.g. 'x = 5')
        case operator_type::op_assignment:
            parse_context.constraints.emplace_back(
                equal_constraint{
                    assigned_node_type(*n.left, parse_context),
                    assigned_node_type(*n.right, parse_context)
                }
            );
            break;
        default:
            break;
    }
}

void type_inference::leave(ast::let_expression& n)
{
    if (!n.init_expression)
    {
        return;
    }

    parse_context.constraints.emplace_back(equal_constraint{parse_context.symbol_at(n.symbol_ref).symbol_type, assigned_node_type(*n.init_expression, parse_context)});
}

void type_inference::enter(ast::function_definition& n)
{
    current_return_type = parse_context.function_return_type(n.function_head.signature.function_type);
}

void type_inference::leave(ast::function_definition& n)
{
    // Normalization turns a trailing value expression into an implicit return
    // after inference, so collect its type constraint here already.
    if (current_return_type.has_value() &&
        !parse_context.is_primitive_type(current_return_type.value(), primitive_type::Void) &&
        !n.code.empty() &&
        !std::holds_alternative<ast::return_expression>(*n.code.back()) &&
        !std::holds_alternative<ast::discard_expression>(*n.code.back()))
    {
        parse_context.constraints.emplace_back(
            equal_constraint{
                current_return_type.value(),
                assigned_node_type(*n.code.back(), parse_context)
            }
        );
    }

    current_return_type.reset();
}

void type_inference::leave(ast::return_expression& n)
{
    if (n.expression && current_return_type.has_value() &&
        !parse_context.is_primitive_type(current_return_type.value(), primitive_type::Void))
    {
        parse_context.constraints.emplace_back(
            equal_constraint{
                current_return_type.value(),
                assigned_node_type(*n.expression, parse_context)
            }
        );
    }
}
