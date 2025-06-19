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
    token parse_token();

    token parse_number();
};