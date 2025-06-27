#pragma once
#include <istream>
#include <optional>
#include <variant>
#include "lexer.hpp"

struct node_number;
struct node_function_call;
struct node_function_definition;
struct node_expression;
struct node_parse_error;

using ast_node = std::variant<node_number, node_function_call, node_function_definition, node_expression, node_parse_error>;

struct node_number
{
    long long number;
    type_kind assigned_type;
};

struct node_function_call
{
    std::string function_name;
    std::unique_ptr<ast_node> parameter;
};

struct node_function_definition
{
    std::string function_name;
    // TODO: parameter definitions;
    // TODO: code should probably be a list of expressions or similar
    std::unique_ptr<ast_node> code;
    type_kind return_type;
};

struct node_expression
{
    std::unique_ptr<ast_node> left, right;
    token_operator::operator_type operation;
    type_kind assigned_type;
};

struct node_parse_error
{
    source_position error_location;
    std::string error_message;
    // A node representing a failed parsing result
};

bool is_error(const ast_node &node);

class parser
{
public:
    explicit parser(lexer &l);

    ast_node parse();

private:
    lexer &capy_lexer;

    ast_node create_error(const std::string& error_message);

    ast_node parse_function_definition();
    ast_node parse_expression(int min_precedence=0);
    ast_node parse_function_call(const std::string function_name);
    ast_node parse_primary();
    ast_node parse_number();
};