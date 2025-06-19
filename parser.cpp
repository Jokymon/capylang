#include "parser.hpp"
#include <string>

lexer::lexer(std::istream &input)
    : input_(input), lookahead_(std::nullopt) {}

const token &lexer::peek_token()
{
    if (!lookahead_)
    {
        lookahead_ = parse_token();
    }
    return *lookahead_;
}

token lexer::next_token()
{
    if (lookahead_)
    {
        token temp = std::move(*lookahead_);
        lookahead_.reset();
        return temp;
    }
    return parse_token();
}

void lexer::skip_whitespace()
{
    while (std::isspace(input_.peek()))
    {
        input_.get();
    }
}

void lexer::skip_comment()
{
    char ch;
    while ((ch = input_.get()) != '\n') {}
}

token lexer::parse_token()
{
    skip_whitespace();

    if (input_.eof())
    {
        return token_eof{};
    }

    char ch = input_.peek();
    if (ch == '/')
    {
        skip_comment();
        skip_whitespace();
    }

    ch = input_.peek();
    if (std::isdigit(ch))
    {
        return parse_number();
    }

    input_.get();
    return token_illegal{std::to_string(ch)};
}

token lexer::parse_number()
{
    std::string num;
    while (std::isdigit(input_.peek()))
    {
        num += input_.get();
    }
    return token_integer{std::stoi(num)};
}