#include "lir.hpp"
#include "tools.hpp"
#include <iostream>

namespace lir
{

    // --------------------------------------------------------

    type_id lir_assigned_node_type(const expr& node, context& ctx)
    {
        return std::visit(
            [&](const auto& n) -> type_id
            {
                using T = std::decay_t<decltype(n)>;

                if constexpr (std::is_same_v<T, number>)
                {
                    return n.assigned_type;
                }
                else if constexpr (std::is_same_v<T, char_literal>)
                {
                    return ctx.intern_primitive(primitive_type::Char);
                }
                else if constexpr (std::is_same_v<T, bool_literal>)
                {
                    return ctx.intern_primitive(primitive_type::Boolean);
                }
                else if constexpr (std::is_same_v<T, string_literal>)
                {
                    return ctx.intern_primitive(primitive_type::String);
                }
                else if constexpr (std::is_same_v<T, store_record_expression>)
                {
                    return ctx.intern_primitive(primitive_type::Void);
                }
                else if constexpr (std::is_same_v<T, load_expression>)
                {
                    return n.assigned_type;
                }
                else if constexpr (std::is_same_v<T, store_expression>)
                {
                    return ctx.intern_primitive(primitive_type::Void);
                }
                else if constexpr (std::is_same_v<T, if_expression>)
                {
                    return n.assigned_type;
                }
                else if constexpr (std::is_same_v<T, while_expression>)
                {
                    return ctx.intern_primitive(primitive_type::Void);
                }
                else if constexpr (std::is_same_v<T, function_call>)
                {
                    const auto& sym = ctx.symbol_at(n.symbol_ref);
                    CAPY_ASSERT(sym.kind == symbol_kind::function, "Compiler Error: LIR function call should resolve to function symbol");

                    auto return_type = ctx.function_return_type(sym.signature.function_type);
                    CAPY_ASSERT(return_type.has_value(), "Compiler Error: LIR function call type should have a valid return type");
                    return return_type.value();
                }
                else if constexpr (std::is_same_v<T, cast_expression>)
                {
                    return n.cast_type;
                }
                else if constexpr (std::is_same_v<T, discard_expression>)
                {
                    return ctx.intern_primitive(primitive_type::Void);
                }
                else if constexpr (std::is_same_v<T, return_expression>)
                {
                    return ctx.intern_primitive(primitive_type::Void);
                }
                else if constexpr (std::is_same_v<T, unary_expression>)
                {
                    return n.assigned_type;
                }
                else if constexpr (std::is_same_v<T, binary_expression>)
                {
                    return n.assigned_type;
                }
                else
                {
                    CAPY_FAIL("Compiler error: LIR encountered unhandled type; index=%lu", node.index());
                    return ILLEGAL_TYPE;
                }
            },
            node
        );
    }

}