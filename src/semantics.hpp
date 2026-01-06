#pragma once
#include "parser.hpp"
#include <optional>

type_kind assigned_node_type(const ast_node &node, const context& ctx);

class semantic_analyser
{
public:
    explicit semantic_analyser(context& ctx);
    void semantic_analysis(node_module &module);

    std::vector<parse_error> errors;

private:
    void append_error_at(source_position location, const std::string &error_message);

    void visit(ast_node &root);
    void process(source_range location, node_number &n);
    void process(source_range location, node_char_literal &n);
    void process(source_range location, node_bool_const &n);
    void process(source_range location, node_string_literal &n);
    void process(source_range location, node_var_reference &n);
    void process(source_range location, node_pointer_deref &n);
    void process(source_range location, node_record_definition &n);
    void process(source_range location, node_record_initialisation &n);
    void process(source_range location, node_field_deref &n);
    void process(source_range location, node_function_call &n);
    void process(source_range location, node_if_expression &n);
    void process(source_range location, node_while_expression &n);
    void process(source_range location, node_let_expression &n);
    void process(source_range location, node_import_definition &n);
    void process(source_range location, node_global &n);
    void process(source_range location, node_function_definition &n);
    void process(source_range location, node_cast_expression &n);
    void process(source_range location, node_expression &n);
    void process(source_range location, node_module &n);

    context& parse_context;
    assign_context current_context;
};
