#include "parser.hpp"
#include <memory>
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
    while ((ch = input_.get()) != '\n')
    {
        if (input_.eof()) {
            return;
        }
    }
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
    else if (ch == '+')
    {
        return parse_operator();
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

token lexer::parse_operator()
{
    input_.get();
    return token_operator{token_operator::op_plus};
}

// ---------------------------------------------------------------------------------------------------

bool is_error(const ast_node& node)
{
    return std::holds_alternative<node_parse_error>(node);
}

parser::parser(lexer &l)
    : capy_lexer(l) {}

ast_node parser::parse()
{
    // TODO: check for EOF after expression
    return parse_expression();
}

ast_node parser::parse_expression()
{
    auto lhs = parse_number();
    if (is_error(lhs)) {
        return lhs;
    }

    while (capy_lexer.ahead_is<token_operator>())
    {
        auto op = capy_lexer.next_token();
        auto rhs = parse_expression();

        lhs = node_expression{
            .left = std::make_unique<ast_node>(std::move(lhs)),
            .right = std::make_unique<ast_node>(std::move(rhs)),
            .operation = {std::get<token_operator>(op).op_type}};
    }

    return lhs;
}

ast_node parser::parse_number()
{
    auto lhs = capy_lexer.expect<token_integer>();
    if (!lhs.has_value())
    {
        return node_parse_error{"Expected a number in the input"};
    }
    return node_number{lhs.value().number};
}