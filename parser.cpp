#include "parser.hpp"
#include <memory>
#include <string>

template <typename T, typename... Args>
ast_node make_located(source_position start, source_position end, Args &&...args)
{
    return ast_node{
        .value = T{std::forward<Args>(args)...},
        .location = source_range{
            .start = start,
            .end = end}};
}

bool is_error(const ast_node &node)
{
    return std::holds_alternative<node_parse_error>(node.value);
}

std::string repr_type(type_kind type_spec)
{
    switch (type_spec)
    {
    case type_kind::unassigned:
        return "!unassigned!";
    case type_kind::void_type:
        return "void";
    case type_kind::s32:
        return "s32";
    case type_kind::u32:
        return "u32";
    }
}

std::optional<type_kind> type_from_id(const std::string &id)
{
    if (id == "u32")
    {
        return type_kind::u32;
    }
    else if ((id == "s32") || (id == ""))
    {
        return type_kind::s32;
    }

    return std::nullopt;
}

parser::parser(lexer &l)
    : capy_lexer(l) {}

ast_node parser::parse()
{
    auto root = parse_module();
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
    auto current_pos = capy_lexer.current_source_position();

    return make_located<node_parse_error>(
        current_pos,
        current_pos,
        current_pos,
        error_message);
}

ast_node parser::parse_module()
{
    auto capy_module = make_located<node_module>(
        source_position{1, 1},
        source_position{1, 1});

    while (capy_lexer.ahead_is<token_symbol>() && capy_lexer.next_as<token_symbol>().sym_type == token_symbol::sym_kw_import)
    {
        auto import_def = parse_import_definition();
        if (is_error(import_def))
        {
            return import_def;
        }

        std::get<node_module>(capy_module.value).imports.push_back(std::make_unique<ast_node>(std::move(import_def)));
    }

    while (capy_lexer.ahead_is<token_symbol>() && capy_lexer.next_as<token_symbol>().sym_type == token_symbol::sym_kw_fn)
    {
        auto function = parse_function_definition();
        if (is_error(function))
        {
            return function;
        }

        std::get<node_module>(capy_module.value).functions.push_back(std::make_unique<ast_node>(std::move(function)));
    }

    return capy_module;
}

std::optional<ast_node> parser::parse_function_signature(function_signature& signature)
{
    if (!capy_lexer.ahead_is<token_identifier>())
    {
        return create_error("Expecting a function name");
    }
    auto [start_range, function_name] = capy_lexer.expect<token_identifier>();
    signature.location.start = start_range.start;
    signature.function_name = function_name.name;

    if (!capy_lexer.expect_symbol(token_symbol::sym_brac_open))
    {
        return create_error("Expecting an opening bracket '(' for function parameters");
    }

    auto error_result = parse_parameters(signature.parameters);
    if (error_result.has_value())
    {
        return std::move(error_result.value());
    }

    signature.location.end = capy_lexer.current_source_position();
    if (!capy_lexer.expect_symbol(token_symbol::sym_brac_close))
    {
        return create_error("Expecting an closing bracket ')' for function parameters");
    }

    std::optional<type_kind> return_type = type_kind::void_type;

    if (capy_lexer.ahead_is<token_symbol>() && (capy_lexer.next_as<token_symbol>().sym_type == token_symbol::sym_arrow))
    {
        capy_lexer.next_token();

        if (!capy_lexer.ahead_is<token_identifier>())
        {
            return create_error("Expecting a return type identifier after ->");
        }
        auto [end_range, return_type_id] = capy_lexer.expect<token_identifier>();
        signature.location.end = end_range.end;

        return_type = type_from_id(return_type_id.name);
        if (!return_type.has_value())
        {
            return create_error("Return type " + return_type_id.name + " is unknown");
        }
    }

    signature.return_type = return_type.value();

    return std::nullopt;
}

std::optional<ast_node> parser::parse_parameters(std::vector<param_spec>& parameters)
{
    while (capy_lexer.ahead_is<token_identifier>()) {
        auto [_, param_name] = capy_lexer.expect<token_identifier>();
        if (!capy_lexer.expect_symbol(token_symbol::sym_colon))
        {
            return create_error("Expecting a colon ':' between parameter name and parameter type");
        }

        auto type_spec_node = parse_type_reference();
        if (is_error(type_spec_node))
        {
            return type_spec_node;
        }

        auto type_spec = std::get<node_type_spec>(type_spec_node.value).type_spec;

        parameters.emplace_back(param_name.name, type_spec);

        // eat the comma between the parameters
        if (capy_lexer.ahead_is<token_symbol>() && capy_lexer.next_as<token_symbol>().sym_type == token_symbol::sym_comma) {
            capy_lexer.expect<token_symbol>();
        }
    }

    return std::nullopt;
}

ast_node parser::parse_import_definition()
{
    // Eat the 'import' keyword that we already established in
    // the calling function
    auto [start_range, _] = capy_lexer.expect<token_symbol>();

    if (!capy_lexer.ahead_is<token_identifier>())
    {
        return create_error("Expecting a namespace identifier");
    }
    auto [_, namespace_name] = capy_lexer.expect<token_identifier>();

    if (!capy_lexer.expect_symbol(token_symbol::sym_dcolon))
    {
        return create_error("Namespace is expected to be followed by '::' and a symbol name");
    }

    function_signature signature;
    auto signature_result = parse_function_signature(signature);
    if (signature_result.has_value())
    {
        return std::move(signature_result.value());
    }

    std::optional<std::string> alias_name;

    if (capy_lexer.ahead_is<token_operator>() && (capy_lexer.next_as<token_operator>().op_type == token_operator::op_conversion))
    {
        capy_lexer.next_token();

        if (!capy_lexer.ahead_is<token_identifier>())
        {
            return create_error("Expecting an alias identifier for imported symbol");
        }
        auto [_, alias_name] = capy_lexer.expect<token_identifier>();
    }

    if (!capy_lexer.ahead_is<token_symbol>() || capy_lexer.next_as<token_symbol>().sym_type != token_symbol::sym_semicolon)
    {
        return create_error("Expecting a closing ';' after import definition");
    }
    auto [end_range, _] = capy_lexer.expect<token_symbol>();

    return make_located<node_import_definition>(
        start_range.start,
        end_range.end,
        namespace_name.name,
        signature,
        alias_name
    );
}

ast_node parser::parse_function_definition()
{
    auto [start_range, _] = capy_lexer.expect<token_symbol>();

    function_signature function_signature;
    auto signature_errors = parse_function_signature(function_signature);
    if (signature_errors.has_value())
    {
        return std::move(signature_errors.value());
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

    if (!capy_lexer.ahead_is<token_symbol>() || capy_lexer.next_as<token_symbol>().sym_type != token_symbol::sym_curly_close)
    {
        return create_error("Expecting a closing brace '}' after function body definition");
    }
    auto [end_range, _] = capy_lexer.expect<token_symbol>();

    return make_located<node_function_definition>(
        start_range.start,
        end_range.end,
        function_signature,
        std::make_unique<ast_node>(std::move(function_body)));
}

ast_node parser::parse_expression(int min_precedence)
{
    if (capy_lexer.ahead_is<token_symbol>() &&
        (capy_lexer.next_as<token_symbol>().sym_type == token_symbol::sym_brac_open))
    {
        // eat up the '(' token
        auto [start, _] = capy_lexer.expect<token_symbol>();

        auto expression = parse_expression();

        if (is_error(expression))
        {
            // Expression parsing already failed, so early return
            return expression;
        }

        // check for a closing )
        if (capy_lexer.ahead_is<token_symbol>() &&
            (capy_lexer.next_as<token_symbol>().sym_type == token_symbol::sym_brac_close))
        {
            auto [end, _] = capy_lexer.expect<token_symbol>();

            if (capy_lexer.ahead_is<token_operator>() && (capy_lexer.next_as<token_operator>().op_type == token_operator::op_conversion))
            {
                // next token is a conversion operator, skip it
                auto op_token = capy_lexer.next_token();

                auto type_spec = parse_type_reference();
                if (is_error(type_spec))
                {
                    return type_spec;
                }

                return make_located<node_expression>(
                    start.start,
                    end.end,
                    std::make_unique<ast_node>(std::move(expression)),
                    std::make_unique<ast_node>(std::move(type_spec)),
                    op_token.location,
                    token_operator::op_conversion,
                    type_kind::unassigned);
            }
            else
            {
                return expression;
            }
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
        source_range start = lhs.location;

        while (capy_lexer.ahead_is<token_operator>())
        {
            auto op = capy_lexer.next_as<token_operator>();
            int prec = op.get_precedence();
            if (prec < min_precedence)
                break;
            auto op_token = capy_lexer.next_token();

            ast_node rhs;
            if (op.op_type == token_operator::op_conversion)
            {
                rhs = parse_type_reference();
            }
            else
            {
                rhs = parse_expression(prec + 1);
            }

            if (is_error(rhs))
            {
                return rhs;
            }
            source_range end = rhs.location;

            lhs = make_located<node_expression>(
                start.start,
                end.end,
                std::make_unique<ast_node>(std::move(lhs)),
                std::make_unique<ast_node>(std::move(rhs)),
                op_token.location,
                op.op_type,
                type_kind::unassigned);
        }

        return lhs;
    }
}

ast_node parser::parse_function_call(const std::string function_name)
{
    // skip over the opening ( of the function call
    auto [start_location, _] = capy_lexer.expect<token_symbol>();

    std::vector<std::unique_ptr<ast_node>> function_parameters;
    while (!capy_lexer.ahead_is<token_symbol>() || capy_lexer.next_as<token_symbol>().sym_type != token_symbol::sym_brac_close) {
        auto parameter = parse_expression();
        if (is_error(parameter))
        {
            return parameter;
        }
        function_parameters.emplace_back(std::make_unique<ast_node>(std::move(parameter)));

        if (capy_lexer.ahead_is<token_symbol>() && capy_lexer.next_as<token_symbol>().sym_type == token_symbol::sym_comma)
        {
            // eat the separating ','
            capy_lexer.expect_symbol(token_symbol::sym_comma);
        }
    }

    // TODO: check for closing )
    auto [end_location, _] = capy_lexer.expect<token_symbol>();

    return make_located<node_function_call>(
        start_location.start,
        end_location.end,
        function_name,
        std::move(function_parameters));
}

ast_node parser::parse_primary()
{
    if (capy_lexer.ahead_is<token_identifier>())
    {
        auto [_, id] = capy_lexer.expect<token_identifier>();

        if (capy_lexer.ahead_is<token_symbol>() &&
            capy_lexer.next_as<token_symbol>().sym_type == token_symbol::sym_brac_open)
        {
            return parse_function_call(id.name);
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

ast_node parser::parse_type_reference()
{
    if (!capy_lexer.ahead_is<token_identifier>())
    {
        return create_error("Expecting an identifier for the type specification");
    }
    auto [token_location, type_name] = capy_lexer.expect<token_identifier>();

    auto type_spec = type_from_id(type_name.name);
    if (!type_spec.has_value())
    {
        return create_error("Unknown type specification: " + type_name.name);
    }

    return make_located<node_type_spec>(
        token_location.start,
        token_location.end,
        type_spec.value());
}

ast_node parser::parse_number()
{
    if (!capy_lexer.ahead_is<token_integer>())
    {
        return create_error("Expected a number in the input");
    }

    auto [lhs_location, lhs] = capy_lexer.expect<token_integer>();

    auto number_type = type_from_id(lhs.type_suffix);
    if (!number_type.has_value())
    {
        return create_error("Number has an illegal suffix");
    }

    return make_located<node_number>(
        lhs_location.start,
        lhs_location.end,
        lhs.number,
        number_type.value());
}