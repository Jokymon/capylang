#include "normalization.hpp"
#include "tools.hpp"

normalization::normalization(context& ctx)
: ast::visitor()
, parse_context(ctx)
{
}

void normalization::normalize(ast::module& module)
{
    visit(module);
}

void normalization::leave(ast::field_deref& n)
{
    if (parse_context.is_pointer_type(n.object_type))
    {
        // turn an implicit pointer dereferencing into an explicit one.
        auto ptr_base_type = parse_context.derefed_type(n.object_type);
        // We have just checked for ptr type before, this should work
        CAPY_ASSERT(ptr_base_type.has_value(), "Fatal compiler error");
        n.object = std::make_unique<ast::expr>(ast::pointer_deref{
            {.location = n.location},
            .pointer_expression = std::move(n.object),
            .assigned_type = n.object_type
        });
        n.object_type = ptr_base_type.value();
    }
}

void normalization::leave(ast::function_definition& n)
{
    auto maybe_return_type = parse_context.function_return_type(n.function_head.signature.function_type);
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

    if (std::holds_alternative<ast::return_expression>(*n.code.back()) ||
        std::holds_alternative<ast::discard_expression>(*n.code.back()))
    {
        return;
    }

    auto tail = std::move(n.code.back());
    const auto tail_location = ast::as_trait<ast::location>(*tail).location;
    n.code.back() = std::make_unique<ast::expr>(
        ast::return_expression{
            {.location = tail_location},
            .expression = std::move(tail),
            .is_explicit = false,
        }
    );
}
