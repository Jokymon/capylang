#pragma once
#include "ast.hpp"
#include "ast_visitor.hpp"
#include "diagnostics.hpp"
#include "parser.hpp"

class type_inference : public ast::visitor, private diagnostic_emitter
{
public:
    explicit type_inference(diagnostic_bag& diagnostics, context& ctx);
    void infer_types(ast::module& module);

private:
    void unify();

    void leave(ast::function_call& n) override;
    void leave(ast::binary_expression& n) override;
    void leave(ast::let_expression& n) override;

    context& parse_context;
};
