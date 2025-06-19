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
    } else if (ch == '+') {
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

parser::parser(lexer &l)
    : capy_lexer(l) {}

ast_node parser::parse()
{
    return parse_expression();
}

ast_node parser::parse_expression()
{
    auto lhs = capy_lexer.expect<token_integer>();
    if (!lhs.has_value())
    {
        return node_parse_error{"Expected a number in the input"};
    }
    if (capy_lexer.ahead_is<token_operator>()) {
        auto op = capy_lexer.next_token();
        auto rhs = capy_lexer.expect<token_integer>();

        return node_expression{
            .left = std::make_unique<node_number>(std::move(lhs.value().number)),
            .right = std::make_unique<node_number>(std::move(rhs.value().number)),
            .operation = std::get<token_operator>(op).op_type
        };
    } else {
        return node_number{lhs.value().number};
    }
}
