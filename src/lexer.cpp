#include "lexer.hpp"
#include <assert.h>

std::string token_symbol::to_string() const
{
    switch (sym_type)
    {
    case sym_kw_as:
        return "as";
    case sym_kw_fn:
        return "fn";
    case sym_kw_import:
        return "import";
    case sym_kw_let:
        return "let";
    case sym_kw_record:
        return "record";
    case sym_arrow:
        return "->";
    case sym_colon:
        return ":";
    case sym_comma:
        return ",";
    case sym_dcolon:
        return "::";
    case sym_dquote:
        return "\"";
    case sym_equal:
        return "=";
    case sym_minus:
        return "-";
    case sym_percent:
        return "%";
    case sym_period:
        return ".";
    case sym_plus:
        return "+";
    case sym_semicolon:
        return ";";
    case sym_slash:
        return "/";
    case sym_star:
        return "*";
    case sym_brac_open:
        return "(";
    case sym_brac_close:
        return ")";
    case sym_curly_open:
        return "{";
    case sym_curly_close:
        return "}";
    }
}

int get_precedence(operator_type op_type)
{
    switch (op_type) {
        case op_conversion:
            return 4;
        case op_multiply:
        case op_division:
        case op_modulus:
            return 3;
        case op_minus:
        case op_plus:
            return 2;
        case op_assignment:
            return 1;
        default:
            return 0;
    }
}

operator_type op_from_symbol(const token_symbol& symbol)
{
    switch (symbol.sym_type) {
        case token_symbol::sym_minus:
            return op_minus;
        case token_symbol::sym_plus:
            return op_plus;
        case token_symbol::sym_star:
            return op_multiply;
        case token_symbol::sym_slash:
            return op_division;
        case token_symbol::sym_percent:
            return op_modulus;
        case token_symbol::sym_equal:
            return op_assignment;
        case token_symbol::sym_kw_as:
            return op_conversion;
        default:
            assert(false && "Unexpected operator symbol");
            return op_conversion;
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

std::string repr_op(operator_type op)
{
    switch (op)
    {
    case op_division:
        return "/";
    case op_multiply:
        return "*";
    case op_modulus:
        return "%";
    case op_minus:
        return "-";
    case op_plus:
        return "+";
    case op_assignment:
        return "=";
    case op_conversion:
        return "as";
    }
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
                          } else if constexpr (std::is_same_v<T, token_string_literal>) {
                            return "<TOK_STR: "+n.str+">";
                          } else if constexpr (std::is_same_v<T, token_symbol>) {
                            return "<TOK_SYM: "+n.to_string()+">";
                          } else {
                            return "<!TOK_FAIL!>";
                          } },
                      tok);
}

lexer::lexer(std::istream &input)
    : input_(input), lookahead_(std::nullopt), look_ahead_position{}, current_position{}
{
}

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

bool lexer::ahead_is_sym(token_symbol::symbol_type symbol)
{
    return ahead_is<token_symbol>() && (next_as<token_symbol>().sym_type == symbol);
}

bool lexer::ahead_is_operator()
{
    const auto &tok = peek_token();
    return std::visit([&](const auto &tok) -> bool {
        using T = std::decay_t<decltype(tok)>;

        if constexpr (std::is_same_v<T, token_symbol>) {
            switch (tok.sym_type) {
                case token_symbol::sym_minus:
                case token_symbol::sym_plus:
                case token_symbol::sym_star:
                case token_symbol::sym_slash:
                case token_symbol::sym_percent:
                case token_symbol::sym_equal:
                case token_symbol::sym_kw_as:
                    return true;
                default:
                    return false;
            }
        } else {
            return false;
        }
    }, tok);
}

bool lexer::expect_symbol(token_symbol::symbol_type symbol)
{
    if (!ahead_is<token_symbol>() || next_as<token_symbol>().sym_type != symbol)
    {
        return false;
    }
    next_token();
    return true;
}

token lexer::next_token()
{
    token temp;
    if (lookahead_)
    {
        temp = std::move(*lookahead_);
        lookahead_.reset();
    }
    else
    {
        temp = parse_token();
    }
    current_position = look_ahead_position;
    return temp;
}

int lexer::peek_ahead()
{
    input_.get();
    int ch = input_.peek();
    input_.unget();

    return ch;
}

int lexer::get_char()
{
    char ch = input_.get();

    if (ch == '\n') {
        look_ahead_position.line += 1;
        look_ahead_position.column = 1;
    }
    else
    {
        look_ahead_position.column += 1;
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
        return token_eof{look_ahead_position};
    }

    char ch = input_.peek();
    while ((ch == '/') && (peek_ahead() == '/'))
    {
        skip_comment();
        skip_whitespace();

        ch = input_.peek();
    }

    if (std::isdigit(ch))
    {
        return parse_number();
    }
    else if ((ch=='-') && (peek_ahead() == '>'))
    {
        auto start_position = look_ahead_position;
        get_char(); get_char();
        return token_symbol{start_position, look_ahead_position, token_symbol::sym_arrow};
    }
    else if (ch=='/')
    {
        get_char();
        return token_symbol{look_ahead_position, look_ahead_position, token_symbol::sym_slash};
    }
    else if (ch=='*')
    {
        get_char();
        return token_symbol{look_ahead_position, look_ahead_position, token_symbol::sym_star};
    }
    else if (ch=='%')
    {
        get_char();
        return token_symbol{look_ahead_position, look_ahead_position, token_symbol::sym_percent};
    }
    else if (ch=='-')
    {
        get_char();
        return token_symbol{look_ahead_position, look_ahead_position, token_symbol::sym_minus};
    }
    else if (ch=='+')
    {
        get_char();
        return token_symbol{look_ahead_position, look_ahead_position, token_symbol::sym_plus};
    }
    else if (ch=='.')
    {
        get_char();
        return token_symbol{look_ahead_position, look_ahead_position, token_symbol::sym_period};
    }
    else if ((ch == ':') && (peek_ahead() == ':'))
    {
        auto start_position = look_ahead_position;
        get_char(); get_char();
        return token_symbol{start_position, look_ahead_position, token_symbol::sym_dcolon};
    }
    else if (ch == ';')
    {
        get_char();
        return token_symbol{look_ahead_position, look_ahead_position, token_symbol::sym_semicolon};
    }
    else if (ch == ':')
    {
        get_char();
        return token_symbol{look_ahead_position, look_ahead_position, token_symbol::sym_colon};
    }
    else if (ch == ',')
    {
        get_char();
        return token_symbol{look_ahead_position, look_ahead_position, token_symbol::sym_comma};
    }
    else if (ch == '=')
    {
        get_char();
        return token_symbol{look_ahead_position, look_ahead_position, token_symbol::sym_equal};
    }
    else if (ch == '(')
    {
        get_char();
        return token_symbol{look_ahead_position, look_ahead_position, token_symbol::sym_brac_open};
    }
    else if (ch == ')')
    {
        get_char();
        return token_symbol{look_ahead_position, look_ahead_position, token_symbol::sym_brac_close};
    }
    else if (ch == '{')
    {
        get_char();
        return token_symbol{look_ahead_position, look_ahead_position, token_symbol::sym_curly_open};
    }
    else if (ch == '}')
    {
        get_char();
        return token_symbol{look_ahead_position, look_ahead_position, token_symbol::sym_curly_close};
    }
    else if (ch == '\"')
    {
        return parse_string_literal();
    }
    else if (is_id_start_character(ch))
    {
        return parse_identifier_or_keyword();
    }

    get_char();
    return token_illegal{look_ahead_position, look_ahead_position, std::to_string(ch)};
}

token lexer::parse_number()
{
    auto start_position = look_ahead_position;
    std::string num;
    while (std::isdigit(input_.peek()))
    {
        num += get_char();
    }

    std::string suffix;
    // we've already eating up all the digits, so if the following character
    // is something of an id (and not a whitespace or something else) then
    // this could be a type suffix
    while (is_id_character(input_.peek()))
    {
        suffix += get_char();
    }

    return token_integer{start_position, look_ahead_position, std::stoi(num), suffix};
}

token lexer::parse_identifier_or_keyword()
{
    auto start_position = look_ahead_position;
    std::string id_name;
    while (is_id_character(input_.peek()))
    {
        id_name += get_char();
    }

    if (id_name == "fn")
    {
        return token_symbol{start_position, look_ahead_position, token_symbol::sym_kw_fn};
    }
    else if (id_name == "import")
    {
        return token_symbol{start_position, look_ahead_position, token_symbol::sym_kw_import};
    }
    else if (id_name == "let")
    {
        return token_symbol{start_position, look_ahead_position, token_symbol::sym_kw_let};
    }
    else if (id_name == "record")
    {
        return token_symbol{start_position, look_ahead_position, token_symbol::sym_kw_record};
    }
    else if (id_name == "as")
    {
        return token_symbol{start_position, look_ahead_position, token_symbol::sym_kw_as};
    }
    else
    {
        return token_identifier{start_position, look_ahead_position, id_name};
    }
}

token lexer::parse_string_literal()
{
    auto start_position = look_ahead_position;
    get_char(); // parse over the opening "
    std::string string_literal;

    while (input_.peek() != '\"')
    {
        auto ch = get_char();

        if (ch == '\\')
        {
            // handle escape sequences
            char next_ch = get_char();
            switch (next_ch)
            {
            case 'n':
                string_literal += '\n';
                break;
            case 't':
                string_literal += '\t';
                break;
            case '\\':
                string_literal += '\\';
                break;
            case '\"':
                string_literal += '\"';
                break;
            default:
                // unknown escape sequence, just add both characters as-is
                string_literal += ch;
                string_literal += next_ch;
                break;
            }
        }
        else
        {
            string_literal += ch;
        }
    }
    get_char(); // parse over the closing "

    return token_string_literal{start_position, look_ahead_position, string_literal};
}