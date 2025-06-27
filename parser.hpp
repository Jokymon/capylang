#pragma once
#include <istream>
#include <optional>
#include <variant>

struct source_position
{
    size_t line = 1;
    size_t column = 1;
};

struct source_range
{
    source_position start;
    source_position end;
};

template <typename T>
struct located
{
    T value;
    source_range location;
};

struct token_integer
{
    long number;
};

struct token_identifier
{
    std::string name;
};

struct token_symbol
{
    enum symbol_type
    {
        sym_kw_fn,

        sym_brac_open,
        sym_brac_close,
        sym_curly_open,
        sym_curly_close,
    };

    symbol_type sym_type;

    std::string to_string() const;
};

struct token_operator
{
    enum operator_type
    {
        op_multiply,
        op_plus,
    };
    operator_type op_type;

    std::string to_string() const;
};

struct token_eof
{
};

struct token_illegal
{
    std::string token_text;
};

using token_raw = std::variant<token_integer, token_identifier, token_operator, token_symbol, token_eof, token_illegal>;
using token = located<token_raw>;

std::string repr_token(const token &tok);

class lexer
{
public:
    explicit lexer(std::istream &input);

    source_position current_source_position() const;

    template <typename T>
    bool ahead_is()
    {
        const token &tok = peek_token();
        return std::holds_alternative<T>(tok.value);
    }

    template <typename T>
    T next_as()
    {
        return std::get<T>(peek_token().value);
    }

    template <typename T>
    std::optional<T> expect()
    {
        if (std::holds_alternative<T>(peek_token().value))
        {
            return std::get<T>(next_token().value);
        }
        return std::nullopt;
    }

    bool expect_symbol(token_symbol::symbol_type symbol);

    token next_token();
    const token &peek_token();

private:
    std::istream &input_;
    std::optional<token> lookahead_;
    source_position current_position;

    int get_char();
    void skip_whitespace();
    void skip_comment();
    token parse_token();

    token parse_number();
    token parse_identifier_or_keyword();
    token parse_operator();
};

// ---------------------------------------------------------------------------------------------

struct node_number;
struct node_function_call;
struct node_function_definition;
struct node_expression;
struct node_parse_error;

using ast_node = std::variant<node_number, node_function_call, node_function_definition, node_expression, node_parse_error>;

struct node_number
{
    int number;
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
};

struct node_expression
{
    std::unique_ptr<ast_node> left, right;
    token_operator::operator_type operation;
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
    ast_node parse_expression();
    ast_node parse_function_call(const std::string function_name);
    ast_node parse_number();
};