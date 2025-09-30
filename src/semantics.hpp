#pragma once
#include "parser.hpp"
#include <optional>

type_kind assigned_node_type(const ast_node &node);

class semantic_analyser
{
public:
    semantic_analyser();
    std::optional<node_parse_error> semantic_analysis(ast_node &root);

private:
    std::optional<node_parse_error> process(node_number &n);
    std::optional<node_parse_error> process(node_var_reference &n);
    std::optional<node_parse_error> process(node_pointer_deref &n);
    std::optional<node_parse_error> process(node_type_spec &n);
    std::optional<node_parse_error> process(source_range location, node_record_initialisation &n);
    std::optional<node_parse_error> process(source_range location, node_field_deref &n);
    std::optional<node_parse_error> process(source_range location, node_function_call &n);
    std::optional<node_parse_error> process(node_let_expression &n);
    std::optional<node_parse_error> process(node_import_definition &n);
    std::optional<node_parse_error> process(node_function_definition &n);
    std::optional<node_parse_error> process(source_range location, node_expression &n);
    std::optional<node_parse_error> process(node_module &n);

    assign_context current_context;
};
