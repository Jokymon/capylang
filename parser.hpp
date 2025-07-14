#pragma once
#include <istream>
#include <optional>
#include <string>
#include <variant>
#include <vector>
#include "lexer.hpp"

enum class type_kind
{
    unassigned,

    void_type,
    s32,
    u32
};

std::string repr_type(type_kind type_spec);

struct node_number;
struct node_type_spec;
struct node_function_call;
struct node_function_definition;
struct node_expression;
struct node_module;
struct node_parse_error;

using ast_node_raw = std::variant<node_number, node_type_spec, node_function_call, node_function_definition, node_expression, node_module, node_parse_error>;
using ast_node = located<ast_node_raw>;

struct node_number
{
    long long number;
    type_kind assigned_type;
};

struct node_type_spec
{
    type_kind type_spec;
};

struct node_function_call
{
    std::string function_name;
    std::vector<std::unique_ptr<ast_node>> parameter;
};

struct param_spec
{
    std::string name;
    type_kind type_spec;
};

struct function_signature
{
    source_range location;

    std::string function_name;
    std::vector<param_spec> parameters;
    type_kind return_type;
};

struct node_function_definition
{
    function_signature signature;
    // TODO: code should probably be a list of expressions or similar
    std::unique_ptr<ast_node> code;
};

struct node_expression
{
    std::unique_ptr<ast_node> left, right;
    source_range op_range;
    token_operator::operator_type operation;
    type_kind assigned_type;
};

struct node_module
{
    std::vector<std::unique_ptr<ast_node>> functions;
};

struct node_parse_error
{
    // TODO: the location is now duplicated from located<ast>
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

    ast_node create_error(const std::string &error_message);

    ast_node parse_module();
    std::optional<ast_node> parse_parameters(std::vector<param_spec>& parameters);
    std::optional<ast_node> parse_function_signature(function_signature& signature);
    ast_node parse_function_definition();
    ast_node parse_expression(int min_precedence = 0);
    ast_node parse_function_call(const std::string function_name);
    ast_node parse_primary();
    ast_node parse_type_reference();
    ast_node parse_number();
};