#pragma once
#include "ast.hpp"
#include "diagnostics.hpp"
#include "parser.hpp"
#include <optional>

type_id assigned_node_type(const node_expr& node, context& ctx);

class semantic_analyser : public ast_visitor, private diagnostic_emitter
{
public:
    explicit semantic_analyser(diagnostic_bag& diagnostics, context& ctx);
    void semantic_analysis(node_module& module);

private:
    void process(source_range location, node_number& n) override;
    void process(source_range location, node_char_literal& n) override;
    void process(source_range location, node_bool_literal& n) override;
    void process(source_range location, node_string_literal& n) override;
    void process(source_range location, node_var_reference& n) override;
    void process(source_range location, node_pointer_deref& n) override;
    void process(source_range location, node_record_definition& n) override;
    void process(source_range location, node_record_initialisation& n) override;
    void process(source_range location, node_field_deref& n) override;
    void process(source_range location, node_function_call& n) override;
    void process(source_range location, node_if_expression& n) override;
    void process(source_range location, node_while_expression& n) override;
    void process(source_range location, node_let_expression& n) override;
    void process(source_range location, node_global_definition& n) override;
    void process(source_range location, node_function_definition& n) override;
    void process(source_range location, node_cast_expression& n) override;
    void process(source_range location, node_discard_expression& n) override;
    void process(source_range location, node_return_expression& n) override;
    void process(source_range location, node_break_statement& n) override;
    void process(source_range location, node_unary_expression& n) override;
    void process(source_range location, node_binary_expression& n) override;
    void process(source_range location, node_import_definition& n) override;
    void process(source_range location, node_module& n) override;

    context& parse_context;

    node_function_head* current_func_head;
};
