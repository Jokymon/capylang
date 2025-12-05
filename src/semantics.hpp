#pragma once
#include "parser.hpp"
#include <optional>

type_kind assigned_node_type(const ast_node &node);

class semantic_analyser
{
public:
    semantic_analyser();
    void semantic_analysis(node_module &module);

    std::vector<parse_error> errors;

private:
    void append_error_at(source_position location, const std::string &error_message);

    void visit(ast_node &root);
    void process(node_number &n);
    void process(source_range location, node_var_reference &n);
    void process(node_pointer_deref &n);
    void process(node_type_spec &n);
    void process(source_range location, node_record_initialisation &n);
    void process(source_range location, node_field_deref &n);
    void process(source_range location, node_function_call &n);
    void process(source_range location, node_if_expression &n);
    void process(source_range location, node_while_expression &n);
    void process(source_range location, node_let_expression &n);
    void process(node_import_definition &n);
    void process(node_function_definition &n);
    void process(source_range location, node_expression &n);
    void process(node_module &n);

    assign_context current_context;
};
