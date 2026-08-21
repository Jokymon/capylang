#pragma once
#include "ast.hpp"
#include "ast_visitor.hpp"
#include "diagnostics.hpp"
#include "parser.hpp"
#include <optional>

type_id assigned_node_type(const ast::expr& node, context& ctx);

class semantic_analyser : public ast::visitor, private diagnostic_emitter
{
public:
    explicit semantic_analyser(diagnostic_bag& diagnostics, context& ctx);
    void semantic_analysis(ast::module& module);

private:
    void leave(ast::number& n) override;
    void leave(ast::var_reference& n) override;
    void leave(ast::pointer_deref& n) override;
    void leave(ast::record_initialisation& n) override;
    void leave(ast::field_deref& n) override;
    void leave(ast::function_call& n) override;
    void leave(ast::if_expression& n) override;
    void enter(ast::while_expression& n) override;
    void leave(ast::while_expression& n) override;
    void leave(ast::let_expression& n) override;
    void enter(ast::function_definition& n) override;
    void leave(ast::function_definition& n) override;
    void leave(ast::return_expression& n) override;
    void leave(ast::break_statement& n) override;
    void leave(ast::unary_expression& n) override;
    void enter(ast::binary_expression& n) override;
    void leave(ast::binary_expression& n) override;

    context& parse_context;
    ast::assign_context_t current_context;
    size_t while_nesting_level;

    ast::function_head* current_func_head;
};
