#include "parser.hpp"
#include <memory>
#include <string>

bool is_operator(char ch)
{
    switch (ch)
    {
    case '+':
    case '*':
        return true;
    default:
        return false;
    }
}

bool is_id_start_character(char ch)
{
    if (ch >= 'a' && ch <= 'z')
    {
        return true;
    }
    if (ch >= 'A' && ch <= 'Z')
    {
        return true;
    }
    if ((ch == '$') || (ch == '_'))
    {
        return true;
    }
    return false;
}

bool is_id_character(char ch)
{
    if (is_id_start_character(ch)) {
        return true;
    }
    if (ch >= '0' && ch <= '9')
    {
        return true;
    }
    return false;
}

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
        if (input_.eof())
        {
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
    else if (is_operator(ch))
    {
        return parse_operator();
    }
    else if (ch == '(')
    {
        input_.get();
        return token_symbol{token_symbol::sym_brac_open};
    }
    else if (ch == ')')
    {
        input_.get();
        return token_symbol{token_symbol::sym_brac_close};
    }
    else if (is_id_start_character(ch))
    {
        return parse_identifier();
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

token lexer::parse_identifier()
{
    std::string id_name;
    while (is_id_character(input_.peek()))
    {
        id_name += input_.get();
    }

    return token_identifier{id_name};
}

token lexer::parse_operator()
{
    char ch = input_.get();
    switch (ch)
    {
    case '*':
        return token_operator{token_operator::op_multiply};
    case '+':
        return token_operator{token_operator::op_plus};
    default:
        // This shouldn't really happen
        return token_illegal{"Unknown operator"};
    }
}

// ---------------------------------------------------------------------------------------------------

bool is_error(const ast_node &node)
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
    if (capy_lexer.ahead_is<token_symbol>() &&
        (capy_lexer.next_as<token_symbol>().sym_type == token_symbol::sym_brac_open))
    {
        // eat up the '(' token
        capy_lexer.next_token();

        auto expression = parse_expression();

        if (is_error(expression))
        {
            // Expression parsing already failed, so early return
            return expression;
        }

        // check for a closing )
        auto closing_brace = capy_lexer.expect<token_symbol>();
        if (closing_brace.has_value() && (closing_brace.value().sym_type == token_symbol::sym_brac_close))
        {
            return expression;
        }

        return node_parse_error{"Expected a closing brace ')' at the end of the expression"};
    }
    else
    {
        if (capy_lexer.ahead_is<token_identifier>())
        {
            auto id = capy_lexer.expect<token_identifier>();

            if (capy_lexer.ahead_is<token_symbol>() &&
                capy_lexer.next_as<token_symbol>().sym_type == token_symbol::sym_brac_open)
            {
                return parse_function_call(id.value().name);
            }
            else {
                return node_parse_error{"Variables are not supported yet"};
            }
        }

        auto lhs = parse_number();
        if (is_error(lhs))
        {
            return lhs;
        }

        while (capy_lexer.ahead_is<token_operator>())
        {
            auto op = capy_lexer.next_token();
            auto rhs = parse_expression();

            lhs = node_expression{
                .left = std::make_unique<ast_node>(std::move(lhs)),
                .right = std::make_unique<ast_node>(std::move(rhs)),
                .operation = std::get<token_operator>(op).op_type};
        }

        return lhs;
    }
}

ast_node parser::parse_function_call(const std::string function_name)
{
    // skip over the opening ( of the function call
    capy_lexer.expect<token_symbol>();

    auto parameter = parse_expression();

    auto call = node_function_call{
        .function_name = function_name,
        .parameter = std::make_unique<ast_node>(std::move(parameter))};

    // TODO: check for closing )
    capy_lexer.expect<token_symbol>();

    return call;
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