#pragma once
#include <istream>
#include <string>
#include <vector>
#include "ast.hpp"
#include "diagnostics.hpp"
#include "lexer.hpp"

class parser : private diagnostic_emitter
{
public:
    explicit parser(diagnostic_bag& diagnostics, std::shared_ptr<lexer> l, context& ctx);

    node_module parse();

private:
    std::shared_ptr<lexer> capy_lexer;
    context& parse_context;

    void append_error(const std::string& error_message);

    void populate_intrinsics();

    node_module parse_module();
    void parse_module_body();
    void parse_attribute();
    void parse_parameters(function_signature& signature, function_type& func_type);
    void parse_function_signature(function_signature& signature);
    node_import_definition parse_import_definition();
    node_global_definition parse_global();
    node_function_definition parse_function_definition();
    node_function_head parse_function_head();
    node_expr parse_expression(int min_precedence = 0);
    node_expr parse_function_call(source_range name_range, const std::string function_name);
    node_expr parse_if_expression();
    node_expr parse_while_expression();
    node_expr parse_let_expression();
    node_expr parse_record_definition();
    node_expr parse_record_initialisation(source_range name_range, const std::string& record_name);
    node_expr parse_field_deref(node_expr object);
    node_expr parse_primary();
    type_id parse_type_reference();
    node_expr parse_number();
    node_expr parse_string();

    void parse_body(std::vector<std::unique_ptr<node_expr>>& body);

    size_t collect_literal(const std::string& literal);

    attribute_bag collected_attributes;
    scope* current_scope;
    node_module* current_module;
    symbol_id error_symbol;
};
