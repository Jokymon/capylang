#pragma once
#include <istream>
#include <optional>
#include <variant>

struct token_integer
{
    long number;
};

struct token_operator
{
    enum operator_type
    {
        op_multiply,
        op_plus,
    };
    operator_type op_type;
};

struct token_eof
{
};

struct token_illegal
{
    std::string token_text;
};

using token = std::variant<token_integer, token_operator, token_eof, token_illegal>;

class lexer
{
public:
    explicit lexer(std::istream &input);

    template <typename T>
    bool ahead_is()
    {
        const token &tok = peek_token();
        return std::holds_alternative<T>(tok);
    }

    template <typename T>
    std::optional<T> expect()
    {
        if (std::holds_alternative<T>(peek_token()))
        {
            return std::get<T>(next_token());
        }
        return std::nullopt;
    }

    token next_token();
    const token &peek_token();

private:
    std::istream &input_;
    std::optional<token> lookahead_;

    void skip_whitespace();
    void skip_comment();
    token parse_token();

    token parse_number();
    token parse_operator();
};

// ---------------------------------------------------------------------------------------------

struct node_expression;
struct node_number;
struct node_parse_error;

using ast_node = std::variant<node_expression, node_number, node_parse_error>;

struct node_number
{
    int number;
};

struct node_expression
{
    std::unique_ptr<ast_node> left, right;
    token_operator::operator_type operation;
};

struct node_parse_error
{
    std::string error_message;
    // A node representing a failed parsing result
};

bool is_error(const ast_node& node);

class parser
{
public:
    explicit parser(lexer &l);

    ast_node parse();

private:
    lexer &capy_lexer;

    ast_node parse_expression();
    ast_node parse_number();
};