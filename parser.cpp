#include "parser.hpp"
#include <memory>
#include <string>

template <typename T, typename... Args>
token make_located(source_position start, source_position end, Args... args)
{
    return token{
        .value = T{args...},
        .location = source_range{
            .start = start,
            .end = end}};
}

std::string token_symbol::to_string() const
{
    switch (sym_type)
    {
    case sym_kw_fn:
        return "SYM_KW_FN";
    case sym_brac_open:
        return "SYM_(";
    case sym_brac_close:
        return "SYM_)";
    case sym_curly_open:
        return "SYM_{";
    case sym_curly_close:
        return "SYM_}";
    }
}

int token_operator::get_precedence() const
{
    switch (op_type) {
        case op_multiply:
            return 2;
        case op_plus:
            return 1;
        default:
            return 0;
    }
}

std::string token_operator::to_string() const
{
    switch (op_type)
    {
    case op_multiply:
        return "OP_MULT";
    case op_plus:
        return "OP_PLUS";
    }
}

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
    if (is_id_start_character(ch))
    {
        return true;
    }
    if (ch >= '0' && ch <= '9')
    {
        return true;
    }
    return false;
}

std::string repr_token(const token &tok)
{
    return std::visit([&](const auto &n) -> std::string
                      {
                          using T = std::decay_t<decltype(n)>;

                          if constexpr (std::is_same_v<T, token_eof>) {
                            return "<TOK_EOF>";
                          } else if constexpr (std::is_same_v<T, token_identifier>) {
                            return "<TOK_ID: "+n.name+">";
                          } else if constexpr (std::is_same_v<T, token_illegal>) {
                            return "<TOK_ILLEGAL: "+n.token_text+">";
                          } else if constexpr (std::is_same_v<T, token_integer>) {
                            return "<TOK_INT: "+std::to_string(n.number)+">";
                          } else if constexpr (std::is_same_v<T, token_operator>) {
                            return "<TOK_OP: "+n.to_string()+">";
                          } else if constexpr (std::is_same_v<T, token_symbol>) {
                            return "<TOK_SYM: "+n.to_string()+">";
                          } else {
                            return "<!TOK_FAIL!>";
                          } },
                      tok.value);
}

lexer::lexer(std::istream &input)
    : input_(input), lookahead_(std::nullopt), current_position{} {}

const token &lexer::peek_token()
{
    if (!lookahead_)
    {
        lookahead_ = parse_token();
    }
    return *lookahead_;
}

source_position lexer::current_source_position() const
{
    return current_position;
}

bool lexer::expect_symbol(token_symbol::symbol_type symbol)
{
    auto t = expect<token_symbol>();
    if (!t.has_value() || t.value().sym_type != symbol)
    {
        return false;
    }
    return true;
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

int lexer::get_char()
{
    char ch = input_.get();

    if (ch == '\n') {
        current_position.line += 1;
        current_position.column = 1;
    }
    else
    {
        current_position.column += 1;
    }

    return ch;
}

void lexer::skip_whitespace()
{
    while (std::isspace(input_.peek()))
    {
        get_char();
    }
}

void lexer::skip_comment()
{
    while (get_char() != '\n')
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
        return make_located<token_eof>(current_position, current_position);
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
        get_char();
        return make_located<token_symbol>(current_position, current_position, token_symbol::sym_brac_open);
    }
    else if (ch == ')')
    {
        get_char();
        return make_located<token_symbol>(current_position, current_position, token_symbol::sym_brac_close);
    }
    else if (ch == '{')
    {
        get_char();
        return make_located<token_symbol>(current_position, current_position, token_symbol::sym_curly_open);
    }
    else if (ch == '}')
    {
        get_char();
        return make_located<token_symbol>(current_position, current_position, token_symbol::sym_curly_close);
    }
    else if (is_id_start_character(ch))
    {
        return parse_identifier_or_keyword();
    }

    get_char();
    return make_located<token_illegal>(current_position, current_position, std::to_string(ch));
}

token lexer::parse_number()
{
    auto start_position = current_position;
    std::string num;
    while (std::isdigit(input_.peek()))
    {
        num += get_char();
    }

    return make_located<token_integer>(start_position, current_position, std::stoi(num));
}

token lexer::parse_identifier_or_keyword()
{
    auto start_position = current_position;
    std::string id_name;
    while (is_id_character(input_.peek()))
    {
        id_name += get_char();
    }

    if (id_name == "fn")
    {
        return make_located<token_symbol>(start_position, current_position, token_symbol::sym_kw_fn);
    }
    else
    {
        return make_located<token_identifier>(start_position, current_position, id_name);
    }
}

token lexer::parse_operator()
{
    char ch = get_char();
    switch (ch)
    {
    case '*':
        return make_located<token_operator>(current_position, current_position, token_operator::op_multiply);
    case '+':
        return make_located<token_operator>(current_position, current_position, token_operator::op_plus);
    default:
        // This shouldn't really happen
        return make_located<token_illegal>(current_position, current_position, "Unknown operator");
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

ast_node parser::create_error(const std::string& error_message)
{
    return node_parse_error{
        .error_location = capy_lexer.current_source_position(),
        .error_message = error_message
    };
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
        .code = std::make_unique<ast_node>(std::move(function_body))};
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
        if (is_error(lhs)) {
            return lhs;
        }

        while (capy_lexer.ahead_is<token_operator>())
        {
            auto op = capy_lexer.next_as<token_operator>();
            int prec = op.get_precedence();
            if (prec < min_precedence) break;

            capy_lexer.next_token();
            auto rhs = parse_expression(prec+1);

            if (is_error(rhs)) {
                return create_error("Incomplete expression, expecting another operand");
            }

            lhs = node_expression{
                .left = std::make_unique<ast_node>(std::move(lhs)),
                .right = std::make_unique<ast_node>(std::move(rhs)),
                .operation = op.op_type};
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
    return node_number{lhs.value().number};
}