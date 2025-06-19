#pragma once
#include <istream>
#include <optional>
#include <variant>

struct token_integer
{
    long number;
};

struct token_eof
{
};

struct token_illegal
{
    std::string token_text;
};

using token = std::variant<token_integer, token_eof, token_illegal>;

class lexer
{
public:
    explicit lexer(std::istream &input);

    token next_token();
    const token &peek_token();

private:
    std::istream &input_;
    std::optional<token> lookahead_;

    void skip_whitespace();
    void skip_comment();
    token parse_token();

    token parse_number();
};

// ---------------------------------------------------------------------------------------------

struct node_number
{
    int number;
};

struct node_parse_error
{
    std::string error_message;
    // A node representing a failed parsing result
};

using ast_node = std::variant<node_number, node_parse_error>;
using node_root_type = node_number;

class parser
{
public:
    explicit parser(lexer &l);

    // TODO: this should probably only either return a AST root
    // node type or an error type. Maybe we could using or typedef
    // a root node type during the development to make the switches
    // a little easier
    ast_node parse();

private:
    lexer &capy_lexer;
};