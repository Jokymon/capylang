#pragma once
#include <istream>
#include <optional>
#include <string>
#include <variant>

struct source_position
{
    size_t line = 1;
    size_t column = 1;
};

struct source_range
{
    source_position start;
    source_position end;
};

template <typename T>
struct located
{
    T value;
    source_range location;
};

struct token_integer
{
    long number;
    std::string type_suffix;
};

struct token_identifier
{
    std::string name;
};

struct token_symbol
{
    enum symbol_type
    {
        sym_kw_fn,

        sym_arrow,

        sym_brac_open,
        sym_brac_close,
        sym_curly_open,
        sym_curly_close,
    };

    symbol_type sym_type;

    std::string to_string() const;
};

struct token_operator
{
    enum operator_type
    {
        op_multiply,
        op_division,
        op_modulus,
        op_plus,
        op_minus,

        op_conversion,
    };
    operator_type op_type;

    int get_precedence() const;

    std::string to_string() const;
};

struct token_eof
{
};

struct token_illegal
{
    std::string token_text;
};

using token_raw = std::variant<token_integer, token_identifier, token_operator, token_symbol, token_eof, token_illegal>;
using token = located<token_raw>;

std::string repr_token(const token &tok);

class lexer
{
public:
    explicit lexer(std::istream &input);

    source_position current_source_position() const;

    template <typename T>
    bool ahead_is()
    {
        const token &tok = peek_token();
        return std::holds_alternative<T>(tok.value);
    }

    template <typename T>
    T next_as()
    {
        return std::get<T>(peek_token().value);
    }

    template <typename T>
    std::optional<T> expect()
    {
        if (std::holds_alternative<T>(peek_token().value))
        {
            return std::get<T>(next_token().value);
        }
        return std::nullopt;
    }

    bool expect_symbol(token_symbol::symbol_type symbol);

    token next_token();
    const token &peek_token();

private:
    std::istream &input_;
    std::optional<token> lookahead_;
    source_position current_position;

    int peek_ahead();
    int get_char();

    void skip_whitespace();
    void skip_comment();
    token parse_token();

    token parse_number();
    token parse_identifier_or_keyword();
    token parse_operator();
};
