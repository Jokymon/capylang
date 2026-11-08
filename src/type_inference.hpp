#pragma once
#include "ast.hpp"
#include "ast_visitor.hpp"
#include "diagnostics.hpp"
#include "parser.hpp"
#include <optional>

class type_inference : public ast::visitor, private diagnostic_emitter
{
public:
    explicit type_inference(diagnostic_bag& diagnostics, context& ctx);
    void infer_types(ast::module& module);

private:
    void unify();
    void default_unresolved_integer_types();

    void leave(ast::function_call& n) override;
    void leave(ast::binary_expression& n) override;
    void leave(ast::let_expression& n) override;
    void enter(ast::function_definition& n) override;
    void leave(ast::function_definition& n) override;
    void leave(ast::return_expression& n) override;

    context& parse_context;
    std::optional<type_id> current_return_type;
};
