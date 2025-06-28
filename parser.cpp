#include "parser.hpp"
#include <memory>
#include <string>

bool is_error(const ast_node &node)
{
    return std::holds_alternative<node_parse_error>(node);
}

std::optional<type_kind> type_from_id(const std::string &id)
{
    if (id == "u32")
    {
        return type_kind::u32;
    }
    else if (id == "")
    {
        return type_kind::s32;
    }

    return std::nullopt;
}

parser::parser(lexer &l)
    : capy_lexer(l) {}

ast_node parser::parse()
{
    auto root = parse_function_definition();
    if (is_error(root))
    {
        return root;
    }
    if (!capy_lexer.ahead_is<token_eof>())
    {
        return create_error("Unexpected trailing code after function definition");
    }
    return root;
}

ast_node parser::create_error(const std::string &error_message)
{
    return node_parse_error{
        .error_location = capy_lexer.current_source_position(),
        .error_message = error_message};
}

ast_node parser::parse_function_definition()
{
    if (!capy_lexer.expect_symbol(token_symbol::sym_kw_fn))
    {
        return create_error("Expecting a function definition starting with keyword 'fn'");
    }

    auto function_name = capy_lexer.expect<token_identifier>();
    if (!function_name.has_value())
    {
        return create_error("Expecting a function name");
    }

    if (!capy_lexer.expect_symbol(token_symbol::sym_brac_open))
    {
        return create_error("Expecting an opening bracket '(' for function parameters");
    }

    if (!capy_lexer.expect_symbol(token_symbol::sym_brac_close))
    {
        return create_error("Expecting an closing bracket ')' for function parameters");
    }

    std::optional<type_kind> return_type = type_kind::void_type;

    if (capy_lexer.ahead_is<token_symbol>() && (capy_lexer.next_as<token_symbol>().sym_type == token_symbol::sym_arrow))
    {
        capy_lexer.next_token();

        auto return_type_id = capy_lexer.expect<token_identifier>();
        if (!return_type_id.has_value())
        {
            return create_error("Expecting a return type identifier after ->");
        }

        return_type = type_from_id(return_type_id.value().name);
        if (!return_type.has_value())
        {
            return create_error("Return type " + return_type_id.value().name + " is unknown");
        }
    }

    if (!capy_lexer.expect_symbol(token_symbol::sym_curly_open))
    {
        return create_error("Expecting an opening brace '{' for function body definition");
    }

    auto function_body = parse_expression();
    if (is_error(function_body))
    {
        return function_body;
    }

    if (!capy_lexer.expect_symbol(token_symbol::sym_curly_close))
    {
        return create_error("Expecting a closing brace ')' after function body definition");
    }

    return node_function_definition{
        .function_name = function_name.value().name,
        .code = std::make_unique<ast_node>(std::move(function_body)),
        .return_type = return_type.value(),
    };
}

ast_node parser::parse_expression(int min_precedence)
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

        return create_error("Expected a closing brace ')' at the end of the expression");
    }
    else
    {
        auto lhs = parse_primary();
        if (is_error(lhs))
        {
            return lhs;
        }

        while (capy_lexer.ahead_is<token_operator>())
        {
            auto op = capy_lexer.next_as<token_operator>();
            int prec = op.get_precedence();
            if (prec < min_precedence)
                break;

            capy_lexer.next_token();
            auto rhs = parse_expression(prec + 1);

            if (is_error(rhs))
            {
                return create_error("Incomplete expression, expecting another operand");
            }

            lhs = node_expression{
                .left = std::make_unique<ast_node>(std::move(lhs)),
                .right = std::make_unique<ast_node>(std::move(rhs)),
                .operation = op.op_type,
                .assigned_type = type_kind::unassigned,
            };
        }

        return lhs;
    }
}

ast_node parser::parse_function_call(const std::string function_name)
{
    // skip over the opening ( of the function call
    capy_lexer.expect<token_symbol>();

    auto parameter = parse_expression();
    if (is_error(parameter))
    {
        return parameter;
    }

    auto call = node_function_call{
        .function_name = function_name,
        .parameter = std::make_unique<ast_node>(std::move(parameter))};

    // TODO: check for closing )
    capy_lexer.expect<token_symbol>();

    return call;
}

ast_node parser::parse_primary()
{
    if (capy_lexer.ahead_is<token_identifier>())
    {
        auto id = capy_lexer.expect<token_identifier>();

        if (capy_lexer.ahead_is<token_symbol>() &&
            capy_lexer.next_as<token_symbol>().sym_type == token_symbol::sym_brac_open)
        {
            return parse_function_call(id.value().name);
        }
        else
        {
            return create_error("Variables are not supported yet");
        }
    }
    else if (capy_lexer.ahead_is<token_integer>())
    {
        return parse_number();
    }
    else
    {
        return create_error("Expected a primary (function call, number, variable)");
    }
}

ast_node parser::parse_number()
{
    auto lhs = capy_lexer.expect<token_integer>();
    if (!lhs.has_value())
    {
        return create_error("Expected a number in the input");
    }

    auto number_type = type_from_id(lhs.value().type_suffix);
    if (!number_type.has_value())
    {
        return create_error("Number has an illegal suffix");
    }

    return node_number{
        .number = lhs.value().number,
        .assigned_type = number_type.value(),
    };
}