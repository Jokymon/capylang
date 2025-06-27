#include "lexer.hpp"

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
    case sym_arrow:
        return "SYM_ARROW";
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
        case op_division:
        case op_modulus:
            return 2;
        case op_minus:
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
    case op_division:
        return "OP_DIV";
    case op_multiply:
        return "OP_MULT";
    case op_modulus:
        return "OP_MOD";
    case op_minus:
        return "OP_MINUS";
    case op_plus:
        return "OP_PLUS";
    }
}

bool is_operator(char ch)
{
    switch (ch)
    {
    case '-':
    case '+':
    case '*':
    case '/':
    case '%':
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
    if ((ch == '/') && (peek_ahead() == '/'))
    {
        skip_comment();
        skip_whitespace();
    }

    ch = input_.peek();
    if (std::isdigit(ch))
    {
        return parse_number();
    }
    else if ((ch=='-') && (peek_ahead() == '>'))
    {
        auto start_position = current_position;
        get_char(); get_char();
        return make_located<token_symbol>(start_position, current_position, token_symbol::sym_arrow);
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

    std::string suffix;
    // we've already eating up all the digits, so if the following character
    // is something of an id (and not a whitespace or something else) then
    // this could be a type suffix
    while (is_id_character(input_.peek()))
    {
        suffix += get_char();
    }

    type_kind number_type = type_kind::s32;
    if (suffix=="u32") {
        number_type = type_kind::u32;
    }

    return make_located<token_integer>(start_position, current_position, std::stoi(num), number_type);
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
    case '/':
        return make_located<token_operator>(current_position, current_position, token_operator::op_division);
    case '*':
        return make_located<token_operator>(current_position, current_position, token_operator::op_multiply);
    case '%':
        return make_located<token_operator>(current_position, current_position, token_operator::op_modulus);
    case '-':
        return make_located<token_operator>(current_position, current_position, token_operator::op_minus);
    case '+':
        return make_located<token_operator>(current_position, current_position, token_operator::op_plus);
    default:
        // This shouldn't really happen
        return make_located<token_illegal>(current_position, current_position, "Unknown operator");
    }
}
