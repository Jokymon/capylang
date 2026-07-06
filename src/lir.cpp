#include "lir.hpp"
#include "tools.hpp"
#include <iostream>

inline std::ostream& operator<<(std::ostream& o, lir_binary_op op)
{
    switch (op)
    {
        case lir_binary_op::multiply:
            o << "*";
            break;
        case lir_binary_op::division:
            o << "/";
            break;
        case lir_binary_op::modulus:
            o << "\"%\"";
            break;
        case lir_binary_op::plus:
            o << "+";
            break;
        case lir_binary_op::minus:
            o << "\"-\"";
            break;
        case lir_binary_op::equals:
            o << "\"==\"";
            break;
        case lir_binary_op::notequals:
            o << "\"!=\"";
            break;
        case lir_binary_op::lessthan:
            o << "\"<\"";
            break;
        case lir_binary_op::lessthan_equal:
            o << "\"<=\"";
            break;
        case lir_binary_op::greaterthan:
            o << "\">\"";
            break;
        case lir_binary_op::greaterthan_equal:
            o << "\">=\"";
            break;
        case lir_binary_op::and_op:
            o << "and";
            break;
        case lir_binary_op::or_op:
            o << "or";
            break;
        case lir_binary_op::shl:
            o << "\"<<\"";
            break;
        case lir_binary_op::shr:
            o << "\">>\"";
            break;
    }
    return o;
}

inline std::ostream& operator<<(std::ostream& o, const lir_place_elem& elem)
{
    std::visit([&](auto&& e)
               {
                    using T = std::decay_t<decltype(e)>;
                    if constexpr (std::is_same_v<T, lir_deref>)
                    {
                        o << "*";
                    }
                    else if constexpr (std::is_same_v<T, lir_field>)
                    {
                        o << "_" << e.index;
                    } },
               elem);
    return o;
}

inline std::ostream& operator<<(std::ostream& o, const lir_place& value)
{
    o << "{ lir_place: { variable: " << value.base.name << ", projection: ";
    for (const auto& proj : value.projection)
    {
        o << proj << ", ";
    }
    o << "} }";
    return o;
}

// lir_val_repr() functions are used in DUMPER_CTX_DUMP_FUNC to dispatch on
// the context-based type to dump
std::string lir_val_repr(const context& ctx, type_id tid)
{
    return ctx.repr(tid);
}

std::string lir_val_repr(const context& ctx, const function_signature& fsig)
{
    return ctx.repr(fsig.function_type);
}

bool lir_attributed_node::has_attribute(const std::string& attr_name) const
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

// --------------------------------------------------------

type_id lir_assigned_node_type(const lir_node& node, context& ctx)
{
    return std::visit(
        [&](const auto& n) -> type_id
        {
            using T = std::decay_t<decltype(n)>;

            if constexpr (std::is_same_v<T, lir_number>)
            {
                return n.assigned_type;
            }
            else if constexpr (std::is_same_v<T, lir_char_literal>)
            {
                return ctx.intern_primitive(primitive_type::Char);
            }
            else if constexpr (std::is_same_v<T, lir_bool_literal>)
            {
                return ctx.intern_primitive(primitive_type::Boolean);
            }
            else if constexpr (std::is_same_v<T, lir_string_literal>)
            {
                return ctx.intern_primitive(primitive_type::String);
            }
            else if constexpr (std::is_same_v<T, lir_store_record_expression>)
            {
                return ctx.intern_primitive(primitive_type::Void);
            }
            else if constexpr (std::is_same_v<T, lir_load_expression>)
            {
                return n.assigned_type;
            }
            else if constexpr (std::is_same_v<T, lir_store_expression>)
            {
                return ctx.intern_primitive(primitive_type::Void);
            }
            else if constexpr (std::is_same_v<T, lir_if_expression>)
            {
                return n.assigned_type;
            }
            else if constexpr (std::is_same_v<T, lir_while_expression>)
            {
                return ctx.intern_primitive(primitive_type::Void);
            }
            else if constexpr (std::is_same_v<T, lir_function_call>)
            {
                const auto& sym = ctx.symbol_at(n.symbol_ref);
                CAPY_ASSERT(sym.kind == symbol_kind::function, "Compiler Error: LIR function call should resolve to function symbol");

                auto return_type = ctx.function_return_type(sym.signature.function_type);
                CAPY_ASSERT(return_type.has_value(), "Compiler Error: LIR function call type should have a valid return type");
                return return_type.value();
            }
            else if constexpr (std::is_same_v<T, lir_cast_expression>)
            {
                return n.cast_type;
            }
            else if constexpr (std::is_same_v<T, lir_discard_expression>)
            {
                return ctx.intern_primitive(primitive_type::Void);
            }
            else if constexpr (std::is_same_v<T, lir_return_expression>)
            {
                return ctx.intern_primitive(primitive_type::Void);
            }
            else if constexpr (std::is_same_v<T, lir_unary_expression>)
            {
                return n.assigned_type;
            }
            else if constexpr (std::is_same_v<T, lir_binary_expression>)
            {
                return n.assigned_type;
            }
            else
            {
                CAPY_FAIL("Compiler error: LIR encountered unhandled type; index=%lu", node.value.index());
                return ILLEGAL_TYPE;
            }
        },
        node.value
    );
}

// --------------------------------------------------------
lir_visitor::lir_visitor()
{
}

void lir_visitor::visit(lir_node& root)
{
    root.value.visit(
        [&](auto& n) -> void
        {
            process(root.location, n);
        }
    );
}

void lir_visitor::visit_nodes(lir_module& module)
{
    for (const auto& import_def : module.imports)
    {
        process(import_def->location, *import_def);
    }
}

void lir_visitor::visit_nodes(lir_function_definition& func_def)
{
    for (const auto& expression : func_def.code)
    {
        visit(*expression);
    }
}

void lir_visitor::visit_nodes(lir_store_record_expression& sr_expr)
{
    for (const auto& init : sr_expr.initialisations)
    {
        visit(*init.value);
    }
}

void lir_visitor::visit_nodes(lir_store_expression& s_expr)
{
    visit(*s_expr.value);
}

void lir_visitor::visit_nodes(lir_if_expression& i_expr)
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

void lir_visitor::visit_nodes(lir_while_expression& w_expr)
{
    visit(*w_expr.condition);

    for (const auto& expression : w_expr.while_code)
    {
        visit(*expression);
    }
}

void lir_visitor::visit_nodes(lir_unary_expression& expr)
{
    visit(*expr.expr);
}

void lir_visitor::visit_nodes(lir_binary_expression& expr)
{
    visit(*expr.left);
    visit(*expr.right);
}

void lir_visitor::visit_nodes(lir_cast_expression& expr)
{
    visit(*expr.expression);
}

void lir_visitor::visit_nodes(lir_discard_expression& expr)
{
    visit(*expr.expression);
}

void lir_visitor::visit_nodes(lir_return_expression& expr)
{
    if (expr.expression)
    {
        visit(*expr.expression);
    }
}

void lir_visitor::visit_nodes(lir_function_call& func_call)
{
    for (const auto& param : func_call.arguments)
    {
        visit(*param);
    }
}

// --------------------------------------------------------

#define DEFINE_DUMP_FUNCS
#define DUMPER_CTX_TYPE const context&
#define DUMPER_CTX_DUMP_FUNC(cntxt, val) lir_val_repr(cntxt, val)
#include "dumpable.hpp"
#include "lir_nodes.hpp"
#undef DEFINE_DUMP_FUNCS

void dump_lir(std::ostream& os, DUMPER_CTX_TYPE ctx, const lir_node& node, int indent)
{
    std::visit([indent, ctx, &os](auto&& arg)
               { dump_lir(os, ctx, arg, indent); },
               node.value);
}