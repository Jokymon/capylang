#pragma once
#include "ast.hpp"
#include "ast_visitor.hpp"
#include "parser.hpp"

class normalization : public ast::visitor
{
public:
    explicit normalization(context& ctx);
    void normalize(ast::module& module);

private:
    void leave(ast::field_deref& n) override;
    void leave(ast::function_definition& n) override;

private:
    context& parse_context;
};
