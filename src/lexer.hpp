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

struct located_token
{
    source_range location;
};

struct token_integer : public located_token
{
    long number;
    std::string type_suffix;
};

struct token_identifier : public located_token
{
    std::string name;
};

struct token_symbol : public located_token
{
    enum symbol_type
    {
        sym_kw_as,
        sym_kw_fn,
        sym_kw_import,
        sym_kw_let,
        sym_kw_record,

        sym_arrow,
        sym_colon,
        sym_comma,
        sym_dcolon,
        sym_equal,
        sym_minus,
        sym_percent,
        sym_period,
        sym_plus,
        sym_semicolon,
        sym_slash,
        sym_star,

        sym_brac_open,
        sym_brac_close,
        sym_curly_open,
        sym_curly_close,
    };

    symbol_type sym_type;

    std::string to_string() const;
};

enum operator_type
{
    op_multiply,
    op_division,
    op_modulus,
    op_plus,
    op_minus,
    op_assignment,

    op_conversion,
};

int get_precedence(operator_type op_type);
operator_type op_from_symbol(const token_symbol& symbol);

struct token_eof : public located_token
{
};

struct token_illegal : public located_token
{
    std::string token_text;
};

using token = std::variant<token_integer, token_identifier, token_symbol, token_eof, token_illegal>;

std::string repr_op(operator_type op);
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
        return std::holds_alternative<T>(tok);
    }
    bool ahead_is_sym(token_symbol::symbol_type symbol);
    bool ahead_is_operator();

    template <typename T>
    T next_as()
    {
        return std::get<T>(peek_token());
    }

    template <typename T>
    T expect()
    {
        if (std::holds_alternative<T>(peek_token()))
        {
            auto token = next_token();
            return std::move(std::get<T>(token));
        }

        // TODO: in reality this function only produces a sensible result
        // when the next token is as expected
        return T{};
    }

    /* This is an expect<>()-variant that will try to get a token based on the expectation
       but will otherwise just return a default constructed token */
    template <typename T>
    T parse_or_default()
    {
        if (std::holds_alternative<T>(peek_token()))
        {
            auto token = next_token();
            return std::move(std::get<T>(token));
        }

        return T{}; // TODO: set the location to current_position
    }

    bool expect_symbol(token_symbol::symbol_type symbol);

    token next_token();
    const token &peek_token();

private:
    std::istream &input_;
    std::optional<token> lookahead_;
    source_position look_ahead_position;
    source_position current_position;

    int peek_ahead();
    int get_char();

    void skip_whitespace();
    void skip_comment();
    token parse_token();

    token parse_number();
    token parse_identifier_or_keyword();
};
