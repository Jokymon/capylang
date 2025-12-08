#include "parser.hpp"
#include <assert.h>
#include <iostream>
#include <memory>
#include <string>

static symbol ERROR_PLACEHOLDER_SYM = symbol{
    "_",
    t_t::u32{},
    function_signature{},
    symbol_kind::global_var,
    false,
    false,
    0};

template <typename T, typename... Args>
ast_node make_located(source_position start, source_position end, Args &&...args)
{
    return ast_node{
        .value = T{std::forward<Args>(args)...},
        .location = source_range{
            .start = start,
            .end = end}};
}

void dump_ast(const ast_node& root, size_t indent=0);

void dump_node(const node_number& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');

    std::cout << ind << "Num:" << n.number << "\n";
}

void dump_node(const node_char_literal& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');

    std::cout << ind << "Char literal: \"" << n.ch << "\"\n";
}

void dump_node(const node_string_literal& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');

    std::cout << ind << "String literal: \"" << n.table_index << "\"\n";
}

void dump_node(const node_bool_const& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');

    std::cout << ind << "Bool:" << n.value << "\n";
}

void dump_node(const node_var_reference& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');

    std::cout << ind << "Var(" << repr_type(n.symbol_ref.get().symbol_type) << "):" << n.name << "\n";
}

void dump_node(const node_pointer_deref& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');

    std::cout << ind << "Pointer Deref:\n";
    dump_ast(*n.pointer_expression, indent+4);
}

void dump_node(const node_let_expression& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');

    std::cout << ind << "Let:\n";
    std::cout << ind << "  " << n.name << "=\n";
    dump_ast(*n.init_expression, indent+4);
}

void dump_node(const node_if_expression& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');
    std::cout << ind << "If\n";
    std::cout << ind << "  Condition:\n";
    dump_ast(*n.condition, indent+4);
    std::cout << ind << "  Then-Body:\n";
    for (const auto& expression : n.then_code)
    {
        dump_ast(*expression, indent+4);
    }
    if (n.else_code.size()>0)
    {
        std::cout << ind << "  Else-Body:\n";
        for (const auto& expression : n.else_code)
        {
            dump_ast(*expression, indent+4);
        }
    }
}

void dump_node(const node_while_expression& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');
    std::cout << ind << "While\n";
    std::cout << ind << "  Condition:\n";
    dump_ast(*n.condition, indent+4);
    std::cout << ind << "  While-Body:\n";
    for (const auto& expression : n.while_code)
    {
        dump_ast(*expression, indent+4);
    }
}

void dump_node(const node_type_spec& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');

    std::cout << ind << "Type: " << repr_type(n.type_spec) << "\n";
}

void dump_node(const node_record_definition& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');

    std::cout << ind << "Record definition " << n.name << ":\n";
    for (const auto& field : n.fields)
    {
        std::cout << ind << "  " << field.name << ": " << repr_type(*field.type_spec) << "\n";
    }
}

void dump_node(const node_record_initialisation& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');

    std::cout << ind << "Record init\n";
    for (const auto& field_init : n.initialisations)
    {
        std::cout << ind << "  " << field_init.field_name << "=\n";
        dump_ast(*field_init.init_expression, indent+6);
    }
}

void dump_node(const node_field_deref& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');

    std::cout << ind << "Deref:\n";
    std::cout << ind << "  object:\n";
    dump_ast(*n.object, indent+4);
    std::cout << ind << "  field: " << n.fieldname << "\n";
}

void dump_node(const node_function_head& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');

    //std::cout << ind << "Function head: TODO\n";
    std::cout << ind << "  Name: " << n.name << "\n";
}

void dump_node(const node_import_definition& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');

    std::cout << ind << "Import definition: TODO\n";
}

void dump_node(const node_global& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');

    std::cout << ind << "Global:\n";
    std::cout << ind << "  " << n.name << "=\n";
    //dump_ast(*n.init_expression, indent+4);
}

void dump_node(const node_function_call& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');

    std::cout << ind << "Function call '" << n.function_name << "'\n";
    for (const auto& param : n.parameter)
    {
        std::cout << ind << "  Parameter:\n";
        dump_ast(*param, indent+4);
    }
}

void dump_node(const node_function_definition& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');

    std::cout << ind << "Function definition:\n";
    dump_node(*n.function_head, indent);
    std::cout << ind << "  Body:\n";
    for (const auto& expression : n.code)
    {
        dump_ast(*expression, indent+4);
    }
}

void dump_node(const node_expression& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');

    std::cout << ind << "Expression; op " << repr_op(n.operation) << "\n";
    
    dump_ast(*n.left, indent+4);
    dump_ast(*n.right, indent+4);
}

void dump_module(const node_module& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');

    std::cout << ind << "Module:\n";
    for (const auto& import : n.imports)
    {
        dump_ast(*import, indent+4);
    }
    for (const auto& type_def : n.typedefs)
    {
        dump_ast(*type_def, indent+4);
    }
    for (const auto& function : n.functions)
    {
        dump_ast(*function, indent+4);
    }
}

void dump_ast(const ast_node& root, size_t indent)
{
    std::visit([=](const auto &n)
        {
            dump_node(n, indent);
        }, root.value);
}

std::string node_field_deref::repr_obj() const
{
    return std::visit([&](const auto &n) -> std::string {
        using T = std::decay_t<decltype(n)>;

        if constexpr (std::is_same_v<T, node_field_deref>) {
            return n.repr_obj() + "." + n.fieldname;
        } else if constexpr (std::is_same_v<T, node_var_reference>) {
            return n.name;
        } else {
            // this branch actually shouldn't happen
            return "";
        }
    }, object->value);
}

std::optional<type_kind> type_from_id(const std::string &id)
{
    if (id == "u32")
    {
        return t_t::u32{};
    }
    else if (id == "u8")
    {
        return t_t::u8{};
    }
    else if ((id == "s32") || (id == ""))
    {
        return t_t::s32{};
    }
    else if (id == "char")
    {
        return t_t::char_type{};
    }
    else if (id == "string")
    {
        return t_t::string{};
    }
    else if (id == "bool")
    {
        return t_t::boolean{};
    }

    return std::nullopt;
}

bool node_function_definition::has_attribute(const std::string& attr_name) const
{
    for (const auto& attr : attributes)
    {
        if (attr.name == attr_name)
        {
            return true;
        }
    }
    return false;
}

parser::parser(lexer &l)
    : capy_lexer(l) {}

node_module parser::parse()
{
    auto root = parse_module();
    if (!capy_lexer.ahead_is<token_eof>())
    {
        append_error("Unexpected trailing code after function definition");
    }
    return root;
}

void parser::append_error(const std::string &error_message)
{
    auto current_pos = capy_lexer.current_source_position();

    errors.emplace_back(parse_error(
        current_pos,
        error_message));
}

void parser::append_error_at(source_position location, const std::string &error_message)
{
    errors.emplace_back(parse_error(
        location,
        error_message));
}

node_module parser::parse_module()
{
    auto capy_module = node_module{
        source_position{1, 1},
        source_position{1, 1}};

    current_module = &capy_module;

    capy_module.module_scope = std::make_unique<scope>();
    current_scope = capy_module.module_scope.get();

    while (capy_lexer.ahead_is_sym(token_symbol::sym_at) ||
            capy_lexer.ahead_is_sym(token_symbol::sym_kw_import) ||
            capy_lexer.ahead_is_sym(token_symbol::sym_kw_global) ||
            capy_lexer.ahead_is_sym(token_symbol::sym_kw_record) ||
            capy_lexer.ahead_is_sym(token_symbol::sym_kw_fn))
    {
        if (capy_lexer.ahead_is_sym(token_symbol::sym_at))
        {
            parse_attribute();
        }
        else if (capy_lexer.ahead_is_sym(token_symbol::sym_kw_import))
        {
            auto import_def = parse_import_definition();

            capy_module.imports.push_back(std::make_unique<ast_node>(std::move(import_def)));
        }
        else if (capy_lexer.ahead_is_sym(token_symbol::sym_kw_global))
        {
            auto global_def = parse_global();
            capy_module.globals.push_back(std::make_unique<ast_node>(std::move(global_def)));
        }
        else if (capy_lexer.ahead_is_sym(token_symbol::sym_kw_record))
        {
            auto record_def = parse_record_definition();
            capy_module.typedefs.push_back(std::make_unique<ast_node>(std::move(record_def)));
        }
        else if (capy_lexer.ahead_is_sym(token_symbol::sym_kw_fn))
        {
            auto function = parse_function_definition();

            capy_module.functions.push_back(std::make_unique<ast_node>(std::move(function)));
        }
    }

    current_module = nullptr;
    return capy_module;
}

void parser::parse_attribute()
{
    capy_lexer.expect_symbol(token_symbol::sym_at);
    auto attribute_name = capy_lexer.expect<token_identifier>();
    collected_attributes.push_back(capy_attribute{attribute_name.name});

    if (capy_lexer.ahead_is_sym(token_symbol::sym_paren_open))
    {
        capy_lexer.expect_symbol(token_symbol::sym_paren_open);
        while (!capy_lexer.ahead_is_sym(token_symbol::sym_paren_close))
        {
            capy_lexer.expect<token_identifier>();
            capy_lexer.expect_symbol(token_symbol::sym_equal);
            parse_primary();

            if (capy_lexer.ahead_is_sym(token_symbol::sym_comma))
            {
                capy_lexer.expect_symbol(token_symbol::sym_comma);
            }
        }
        capy_lexer.expect_symbol(token_symbol::sym_paren_close);
    }
}

void parser::parse_function_signature(function_signature &signature)
{
    if (!capy_lexer.expect_symbol(token_symbol::sym_paren_open))
    {
        append_error("Expecting an opening bracket '(' for function parameters");
    }

    parse_parameters(signature.parameters);

    if (!capy_lexer.expect_symbol(token_symbol::sym_paren_close))
    {
        append_error("Expecting an closing bracket ')' for function parameters");
    }

    std::optional<type_kind> return_type = t_t::void_type{};

    if (capy_lexer.ahead_is_sym(token_symbol::sym_arrow))
    {
        capy_lexer.next_token();

        auto type_spec_node = parse_type_reference();
        return_type = std::get<node_type_spec>(type_spec_node.value).type_spec;
    }

    signature.return_type = return_type.value();
}

void parser::parse_parameters(std::vector<param_spec> &parameters)
{
    while (capy_lexer.ahead_is<token_identifier>())
    {
        auto param_name = capy_lexer.expect<token_identifier>();
        if (!capy_lexer.expect_symbol(token_symbol::sym_colon))
        {
            append_error("Expecting a colon ':' between parameter name and parameter type");
        }

        auto type_spec_node = parse_type_reference();
        auto type_spec = std::get<node_type_spec>(type_spec_node.value).type_spec;

        parameters.emplace_back(param_name.name, type_spec);

        // eat the comma between the parameters
        if (capy_lexer.ahead_is_sym(token_symbol::sym_comma))
        {
            capy_lexer.expect<token_symbol>();
        }
    }
}

ast_node parser::parse_import_definition()
{
    // Eat the 'import' keyword that we already established in
    // the calling function
    auto start_range = capy_lexer.expect<token_symbol>().location;

    if (!capy_lexer.ahead_is<token_identifier>())
    {
        append_error("Expecting a namespace identifier");
    }
    auto namespace_name = capy_lexer.expect<token_identifier>();

    if (!capy_lexer.expect_symbol(token_symbol::sym_dcolon))
    {
        append_error("Namespace is expected to be followed by '::' and a symbol name");
    }

    auto function_head = parse_function_head();

    std::optional<std::string> alias_name;

    if (capy_lexer.ahead_is_sym(token_symbol::sym_kw_as))
    {
        capy_lexer.next_token();

        if (!capy_lexer.ahead_is<token_identifier>())
        {
            append_error("Expecting an alias identifier for imported symbol");
        }
        auto alias_name = capy_lexer.expect<token_identifier>();
    }

    if (!capy_lexer.ahead_is_sym(token_symbol::sym_semicolon))
    {
        append_error("Expecting a closing ';' after import definition");
    }
    auto end_range = capy_lexer.expect<token_symbol>().location;

    return make_located<node_import_definition>(
        start_range.start,
        end_range.end,
        namespace_name.name,
        std::make_unique<node_function_head>(std::move(function_head)),
        alias_name);
}

ast_node parser::parse_global()
{
    // eat the 'global' keyword
    auto start_location = capy_lexer.expect<token_symbol>().location;

    bool is_mutable = false;
    if (capy_lexer.ahead_is_sym(token_symbol::sym_kw_mut))
    {
        // eat the 'mut' keyword
        capy_lexer.expect<token_symbol>();
        is_mutable = true;
    }

    if (!capy_lexer.ahead_is<token_identifier>())
    {
        append_error("Expecting a variable name after 'global' keyword");
    }
    auto variable_name = capy_lexer.parse_or_default<token_identifier>();

    if (!capy_lexer.expect_symbol(token_symbol::sym_colon))
    {
        append_error("Expecting a ':' after variable name in 'global' expression");
    }

    auto type_spec_node = parse_type_reference();
    auto type_spec = std::get<node_type_spec>(type_spec_node.value).type_spec;

    auto new_symbol = symbol{
        .name = variable_name.name,
        .symbol_type = type_spec,
        .kind = symbol_kind::global_var,
        .mutab = is_mutable,
        .is_assigned = false,
        .index_addr = current_scope->symbol_table.size(),
    };
    current_scope->symbol_table[variable_name.name] = new_symbol;

    int32_t init_value = 0;
    if (!capy_lexer.ahead_is_sym(token_symbol::sym_equal))
    {
        append_error("Globals need an initialisation value");
    }
    else
    {
        capy_lexer.expect_symbol(token_symbol::sym_equal);

        auto initializer = parse_number();
        init_value = std::get<node_number>(initializer.value).number;
        current_scope->symbol_table[variable_name.name].is_assigned = true;
    }

    // eat the final semicolon
    auto end_token = capy_lexer.expect<token_symbol>();

    return make_located<node_global>(
        start_location.start,
        end_token.location.end,
        variable_name.name,
        type_spec,
        current_scope->symbol_table[variable_name.name],
        init_value);
}

ast_node parser::parse_function_definition()
{
    auto attributes = collected_attributes;
    collected_attributes.clear();

    auto start_token = capy_lexer.expect<token_symbol>();
    auto start_range = start_token.location;

    auto function_head = parse_function_head();

    if (!capy_lexer.expect_symbol(token_symbol::sym_curly_open))
    {
        append_error("Expecting an opening brace '{' for function body definition");
    }

    auto func_scope = std::make_unique<scope>();
    func_scope->parent = current_scope;
    current_scope = func_scope.get();

    uint32_t argument_index = 0;
    for (const auto& param_spec : function_head.signature.parameters)
    {
        current_scope->symbol_table[param_spec.name] = symbol{
        .name = param_spec.name,
        .symbol_type = param_spec.type_spec,
        .kind = symbol_kind::argument,
        .mutab = false,
        .is_assigned = true,    // function parameters are 'assigned' from the function call
        .index_addr = argument_index++};
    }

    std::vector<std::unique_ptr<ast_node>> function_body;
    parse_body(function_body);
    auto end_range = capy_lexer.expect<token_symbol>().location;

    current_scope = current_scope->parent;

    return make_located<node_function_definition>(
        start_range.start,
        end_range.end,
        attributes,
        std::make_unique<node_function_head>(std::move(function_head)),
        std::move(function_body),
        std::move(func_scope)
    );
}

node_function_head parser::parse_function_head()
{
    if (!capy_lexer.ahead_is<token_identifier>())
    {
        append_error("Expecting a function name");
    }
    auto function_name = capy_lexer.parse_or_default<token_identifier>();
    auto start_range = function_name.location;

    function_signature signature;
    parse_function_signature(signature);

    current_scope->symbol_table[function_name.name] = symbol{
        .name = function_name.name,
        .symbol_type = t_t::unassigned{},
        .signature = signature,
        .kind = symbol_kind::function,
        .mutab = false,
        .index_addr = 0
    };

    return node_function_head{
        start_range.start,
        start_range.end, // TODO: this should span the whole signature
        function_name.name,
        signature};
}

ast_node parser::parse_expression(int min_precedence)
{
    if (capy_lexer.ahead_is_sym(token_symbol::sym_paren_open))
    {
        // eat up the '(' token
        auto start = capy_lexer.expect<token_symbol>().location;

        auto expression = parse_expression();

        // check for a closing )
        if (capy_lexer.ahead_is_sym(token_symbol::sym_paren_close))
        {
            auto end = capy_lexer.expect<token_symbol>().location;

            if (capy_lexer.ahead_is_sym(token_symbol::sym_kw_as))
            {
                // next token is a conversion operator, skip it
                auto op_token = capy_lexer.expect<token_symbol>();

                auto type_spec = parse_type_reference();

                return make_located<node_expression>(
                    start.start,
                    end.end,
                    std::make_unique<ast_node>(std::move(expression)),
                    std::make_unique<ast_node>(std::move(type_spec)),
                    op_token.location,
                    op_conversion,
                    t_t::unassigned{}
                );
            }
            else
            {
                return expression;
            }
        }

        append_error("Expected a closing brace ')' at the end of the expression");

        // return a dummy expression so we can continue parsing
        return make_located<node_expression>(
            start.start,
            start.end,
            std::unique_ptr<ast_node>(nullptr),
            std::unique_ptr<ast_node>(nullptr),
            source_range{start.start, start.end},
            op_multiply,
            t_t::unassigned{}
        );
    }
    else if (capy_lexer.ahead_is_sym(token_symbol::sym_kw_if))
    {
        return parse_if_expression();
    }
    else if (capy_lexer.ahead_is_sym(token_symbol::sym_kw_while))
    {
        return parse_while_expression();
    }
    else if (capy_lexer.ahead_is_sym(token_symbol::sym_kw_let))
    {
        return parse_let_expression();
    }
    else
    {
        auto lhs = parse_primary();
        source_range start = lhs.location;

        while (capy_lexer.ahead_is_operator())
        {
            auto op = op_from_symbol(capy_lexer.next_as<token_symbol>());
            int prec = get_precedence(op);
            if (prec < min_precedence)
                break;
            auto op_token = capy_lexer.expect<token_symbol>();

            ast_node rhs;
            if (op == op_conversion)
            {
                rhs = parse_type_reference();
            }
            else
            {
                rhs = parse_expression(prec + 1);
            }
            source_range end = rhs.location;

            lhs = make_located<node_expression>(
                start.start,
                end.end,
                std::make_unique<ast_node>(std::move(lhs)),
                std::make_unique<ast_node>(std::move(rhs)),
                op_token.location,
                op,
                t_t::unassigned{}
            );
        }

        return lhs;
    }
}

ast_node parser::parse_function_call(source_range name_range, const std::string function_name)
{
    // skip over the opening ( of the function call
    capy_lexer.expect<token_symbol>();

    std::vector<std::unique_ptr<ast_node>> function_parameters;
    while (!capy_lexer.ahead_is_sym(token_symbol::sym_paren_close))
    {
        auto parameter = parse_expression();
        function_parameters.emplace_back(std::make_unique<ast_node>(std::move(parameter)));

        if (capy_lexer.ahead_is_sym(token_symbol::sym_comma))
        {
            // eat the separating ','
            capy_lexer.expect_symbol(token_symbol::sym_comma);
        }
    }

    // TODO: check for closing )
    auto end_location = capy_lexer.expect<token_symbol>().location;

    auto func = current_scope->lookup_function(function_name);
    if (!func.has_value())
    {
        append_error("Function '" + function_name + "' is not defined");

        symbol dummy_function_symbol;
        return make_located<node_function_call>(
            name_range.start,
            end_location.end,
            function_name,
            dummy_function_symbol,
            std::move(function_parameters));
    }

    return make_located<node_function_call>(
        name_range.start,
        end_location.end,
        function_name,
        func.value(),
        std::move(function_parameters));
}

ast_node parser::parse_if_expression()
{
    // eat the 'if' keyword
    auto start_location = capy_lexer.expect<token_symbol>().location;

    auto condition = parse_expression();

    if (!capy_lexer.expect_symbol(token_symbol::sym_curly_open))
    {
        append_error("Expecting an opening brace '{' for if-body");
    }

    std::vector<std::unique_ptr<ast_node>> then_body;
    std::vector<std::unique_ptr<ast_node>> else_body;
    parse_body(then_body);

    // eat the closing '}' of the then-branch
    auto end_range = capy_lexer.expect<token_symbol>().location;

    if (capy_lexer.ahead_is_sym(token_symbol::sym_kw_else))
    {
        // eat the 'else' keyword
        auto start_location = capy_lexer.expect<token_symbol>().location;

        if (!capy_lexer.expect_symbol(token_symbol::sym_curly_open))
        {
            append_error("Expecting an opening brace '{' for the else-body");
        }

        parse_body(else_body);

        end_range = capy_lexer.expect<token_symbol>().location;
    }

    return make_located<node_if_expression>(
        start_location.start,
        start_location.end,
        std::make_unique<ast_node>(std::move(condition)),
        std::move(then_body),
        std::move(else_body),
        t_t::unassigned{}
    );
}

ast_node parser::parse_while_expression()
{
    // eat the 'while' keyword
    auto start_location = capy_lexer.expect<token_symbol>().location;

    auto condition = parse_expression();

    if (!capy_lexer.expect_symbol(token_symbol::sym_curly_open))
    {
        append_error("Expecting an opening brace '{' for while-body");
    }

    std::vector<std::unique_ptr<ast_node>> while_body;
    parse_body(while_body);

    // eat the closing '}' of the while block
    auto end_range = capy_lexer.expect<token_symbol>().location;

    return make_located<node_while_expression>(
        start_location.start,
        start_location.end,
        std::make_unique<ast_node>(std::move(condition)),
        std::move(while_body)
    );
}

ast_node parser::parse_let_expression()
{
    // eat the 'let' keyword
    auto start_location = capy_lexer.expect<token_symbol>().location;

    bool is_mutable = false;
    if (capy_lexer.ahead_is_sym(token_symbol::sym_kw_mut))
    {
        // eat the 'mut' keyword
        capy_lexer.expect<token_symbol>();
        is_mutable = true;
    }

    if (!capy_lexer.ahead_is<token_identifier>())
    {
        append_error("Expecting a variable name after 'let' keyword");
    }
    auto variable_name = capy_lexer.parse_or_default<token_identifier>();

    if (!capy_lexer.expect_symbol(token_symbol::sym_colon))
    {
        append_error("Expecting a ':' after variable name in 'let' expression");
    }

    auto type_spec_node = parse_type_reference();
    auto type_spec = std::get<node_type_spec>(type_spec_node.value).type_spec;

    auto new_symbol = symbol{
        .name = variable_name.name,
        .symbol_type = type_spec,
        .kind = symbol_kind::local_var,
        .mutab = is_mutable,
        .is_assigned = false,
        .index_addr = current_scope->symbol_table.size(),
    };
    current_scope->symbol_table[variable_name.name] = new_symbol;

    if (capy_lexer.ahead_is_sym(token_symbol::sym_equal))
    {
        capy_lexer.expect_symbol(token_symbol::sym_equal);

        auto initializer = parse_expression();

        current_scope->symbol_table[variable_name.name].is_assigned = true;
        return make_located<node_let_expression>(
            start_location.start,
            initializer.location.end,
            variable_name.name,
            type_spec,
            current_scope->symbol_table[variable_name.name],
            std::make_unique<ast_node>(std::move(initializer)));
    }

    return make_located<node_let_expression>(
        start_location.start,
        type_spec_node.location.end,
        variable_name.name,
        type_spec,
        new_symbol,
        nullptr
    );
}

ast_node parser::parse_record_definition()
{
    // eat the 'record' keyword
    auto start_range = capy_lexer.expect<token_symbol>().location;

    if (!capy_lexer.ahead_is<token_identifier>())
    {
        append_error("'record' definition requires an identifier for the record");
    }

    auto record_id = capy_lexer.expect<token_identifier>();

    if (!capy_lexer.expect_symbol(token_symbol::sym_curly_open))
    {
        append_error("Expecting an opening brace '{' starting the record definition");
    }
    
    std::vector<t_t::record::field_spec> new_record_fields;
    while (capy_lexer.ahead_is<token_identifier>())
    {
        auto field_id = capy_lexer.expect<token_identifier>();
        if (!capy_lexer.expect_symbol(token_symbol::sym_colon))
        {
            append_error("Expecting a colon ':' after field name and before type specification");
        }

        auto type_spec_node = parse_type_reference();
        auto type_spec = std::get<node_type_spec>(type_spec_node.value).type_spec;

        new_record_fields.emplace_back(t_t::record::field_spec{field_id.name, std::make_unique<type_kind>(type_spec)});

        if (!capy_lexer.expect_symbol(token_symbol::sym_comma))
        {
            append_error("Record field definitions must be terminated with a ','");
        }
    }

    if (!capy_lexer.ahead_is_sym(token_symbol::sym_curly_close))
    {
        append_error("Record definition must be closed with a matching '}'");
    }
    capy_lexer.expect<token_symbol>();
    if (!capy_lexer.ahead_is_sym(token_symbol::sym_semicolon))
    {
        append_error("Record definition must be terminated with a semicolon");
    }
    auto end_range = capy_lexer.expect<token_symbol>().location;

    auto* global_scope = current_scope->get_global_scope();
    global_scope->symbol_table[record_id.name] = symbol{
        .symbol_type = t_t::record(new_record_fields),
        .kind = symbol_kind::type_spec
    };

    return make_located<node_record_definition>(
        start_range.start,
        end_range.end,
        record_id.name,
        new_record_fields
    );
}

ast_node parser::parse_record_initialisation(source_range name_range, const std::string& record_name)
{
    auto record_type = current_scope->lookup_type(record_name);
    if (!record_type.has_value())
    {
        append_error("Trying to initialise record of unknown type '"+record_name+"'");
        record_type = t_t::void_type{};
    }

    // skipping the opening '{'
    capy_lexer.expect<token_symbol>();

    std::vector<field_initialisation> fields;

    while (capy_lexer.ahead_is<token_identifier>())
    {
        auto field_name = capy_lexer.expect<token_identifier>();
        auto field_position = field_name.location;

        if (!capy_lexer.ahead_is_sym(token_symbol::sym_equal))
        {
            append_error("For field initialisation, field name must be followed by a '=' and an initialisation expression");
        }
        capy_lexer.expect<token_symbol>();

        ast_node init_expression = parse_expression();

        if (!capy_lexer.ahead_is_sym(token_symbol::sym_comma))
        {
            append_error("Field initialisations must be terminated by a comma");
        }
        capy_lexer.expect<token_symbol>();
        
        fields.emplace_back(field_initialisation{field_position.start, field_name.name, std::make_unique<ast_node>(std::move(init_expression))});
    }

    if (!capy_lexer.ahead_is_sym(token_symbol::sym_curly_close))
    {
        append_error("Record initialisation must be finished with a closing '}'");
    }
    auto end_range = capy_lexer.expect<token_symbol>().location;

    return make_located<node_record_initialisation>(
        name_range.start,
        end_range.end,
        record_type.value(),
        std::move(fields)
    );
}

ast_node parser::parse_field_deref(type_kind base_type, ast_node object)
{
    // eat the dot '.'
    capy_lexer.expect<token_symbol>();

    if (!capy_lexer.ahead_is<token_identifier>())
    {
        append_error("Missing field name after '.'");
        return make_located<node_field_deref>(
            object.location.start,
            object.location.end,
            std::make_unique<ast_node>(std::move(object)),
            "",
            base_type
        );
    }
    auto field_name = capy_lexer.expect<token_identifier>();
    auto field_range = field_name.location;

    auto node = make_located<node_field_deref>(
        object.location.start,
        field_range.end,
        std::make_unique<ast_node>(std::move(object)),
        field_name.name,
        base_type
    );

    if (capy_lexer.ahead_is_sym(token_symbol::sym_period))
    {
        if (!std::holds_alternative<t_t::record>(base_type))
        {
            // TODO: when are these errors even triggered? In the current tests
            // such errors are only reported from semantic checks
            append_error("Can't dereference field '"+field_name.name+"'");
            // After the error, just return what we have so far
            return node;
        }
        t_t::record base_record_type = std::get<t_t::record>(base_type);
        auto field_type = base_record_type.field_type(field_name.name);
        if (!field_type.has_value())
        {
            // TODO: when are these errors even triggered? In the current tests
            // such errors are only reported from semantic checks
            append_error("Record doesn't contain a field named '"+field_name.name+"'");
            return node;
        }

        node = parse_field_deref(field_type.value(), std::move(node));
    }

    return node;
}

ast_node parser::parse_primary()
{
    if (capy_lexer.ahead_is<token_identifier>())
    {
        auto id = capy_lexer.expect<token_identifier>();
        auto id_range = id.location;

        if (capy_lexer.ahead_is_sym(token_symbol::sym_paren_open))
        {
            return parse_function_call(id_range, id.name);
        }
        // record initialisation and 'if' expressions can look very similar but we
        // can clearly distinguish them on a semantic level. When the identifier
        // before the opening '{' is a type, then this must be a record initialisation
        else if (current_scope->lookup_type(id.name).has_value() &&
                 capy_lexer.ahead_is_sym(token_symbol::sym_curly_open))
        {
            return parse_record_initialisation(id_range, id.name);
        }
        else if (capy_lexer.ahead_is_sym(token_symbol::sym_period))
        {
            auto var = current_scope->lookup(id.name);
            if (!var.has_value())
            {
                append_error_at(id_range.start, "Undefined variable: '"+id.name+"'");
                var = ERROR_PLACEHOLDER_SYM;
            }

            ast_node object = make_located<node_var_reference>(
                id_range.start,
                id_range.end,
                id.name,
                var.value(),
                assign_context::rhs);
            auto var_type = var.value().get().symbol_type;

            return parse_field_deref(var_type, std::move(object));
        }
        else
        {
            auto var = current_scope->lookup(id.name);
            if (!var.has_value())
            {
                if ((id.name == "true") || (id.name == "false")) {
                    return make_located<node_bool_const>(
                        id_range.start,
                        id_range.end,
                        node_bool_const::from_string(id.name));
                }

                append_error_at(id_range.start, "Undefined variable: '"+id.name+"'");
                var = ERROR_PLACEHOLDER_SYM;
            }

            return make_located<node_var_reference>(
                id_range.start,
                id_range.end,
                id.name,
                var.value(),
                assign_context::rhs);
        }
    }
    else if (capy_lexer.ahead_is_sym(token_symbol::sym_star))
    {
        auto op_token = capy_lexer.expect<token_symbol>();

        auto pointer_expr = parse_primary();

        return make_located<node_pointer_deref>(
            op_token.location.start,
            pointer_expr.location.end,
            std::make_unique<ast_node>(std::move(pointer_expr)),
            t_t::unassigned{},
            assign_context::rhs
        );
    }
    else if (capy_lexer.ahead_is<token_integer>())
    {
        return parse_number();
    }
    else if (capy_lexer.ahead_is<token_string_literal>())
    {
        auto literal = capy_lexer.expect<token_string_literal>();
        auto literal_location = literal.location;
        auto literal_index = collect_literal(literal.str);
        return make_located<node_string_literal>(
            literal_location.start,
            literal_location.end,
            literal_index,
            literal.str.size());
    }
    else if (capy_lexer.ahead_is<token_char_literal>())
    {
        auto literal = capy_lexer.expect<token_char_literal>();
        auto literal_location = literal.location;
        return make_located<node_char_literal>(
            literal_location.start,
            literal_location.end,
            literal.ch);
    }
    else
    {
        append_error("Expected a primary (function call, number, variable)");
        // just create one of the simplest primaries in case we couldn't parse one
        return make_located<node_number>(
            capy_lexer.current_source_position(),
            capy_lexer.current_source_position(),
            0,
            t_t::u32{});
    }
}

ast_node parser::parse_type_reference()
{
    bool is_pointer = false;
    if (capy_lexer.ahead_is_sym(token_symbol::sym_star))
    {
        is_pointer = true;
        capy_lexer.next_token();
    }

    source_range token_location;
    token_location.start = capy_lexer.current_source_position();
    token_location.end = capy_lexer.current_source_position();
    token_identifier type_name = {token_location, ""};

    if (capy_lexer.ahead_is<token_identifier>())
    {
        type_name = capy_lexer.expect<token_identifier>();
        token_location = type_name.location;
    }
    else
    {
        append_error("Expecting an identifier for the type specification");
    }

    auto type_spec = type_from_id(type_name.name);
    if (!type_spec.has_value())
    {
        type_spec = current_scope->lookup_type(type_name.name);
        if (!type_spec.has_value())
        {
            append_error("Unknown type specification: " + type_name.name);
            type_spec = t_t::unassigned{};
        }
    }

    if (is_pointer)
    {
        type_spec = t_t::pointer{type_spec.value()};
    }

    return make_located<node_type_spec>(
        token_location.start,
        token_location.end,
        type_spec.value());
}

ast_node parser::parse_number()
{
    auto lhs = capy_lexer.expect<token_integer>();
    auto lhs_location = lhs.location;

    auto number_type = type_from_id(lhs.type_suffix);
    if (!number_type.has_value())
    {
        append_error("Number has an illegal suffix");
        return make_located<node_number>(
            capy_lexer.current_source_position(),
            lhs_location.end,
            0,
            t_t::u32{});
    }

    return make_located<node_number>(
        lhs_location.start,
        lhs_location.end,
        lhs.number,
        number_type.value());
}

void parser::parse_body(std::vector<std::unique_ptr<ast_node>>& body)
{
    while (!capy_lexer.ahead_is_sym(token_symbol::sym_curly_close) &&
           !capy_lexer.ahead_is<token_eof>())
    {
        auto expression = parse_expression();
        body.emplace_back(std::make_unique<ast_node>(std::move(expression)));

        if (capy_lexer.ahead_is_sym(token_symbol::sym_semicolon))
        {
            capy_lexer.expect_symbol(token_symbol::sym_semicolon);

            // If the previous expression wasn't the last one in the body, then it
            // will have left a value on the stack and will make the WASM validation
            // unhappy. So we have to "convert" the type of the last expression to 'void'
            // so the emitter can insert a 'drop' instruction
            auto previous_expression = std::move(body.back());
            body.pop_back();
        
            auto drop_wrapper = make_located<node_expression>(
                previous_expression->location.start,
                previous_expression->location.end,
                std::move(previous_expression),
                std::make_unique<ast_node>(make_located<node_type_spec>(
                    previous_expression->location.start,
                    previous_expression->location.end,
                    t_t::void_type{}
                )),
                previous_expression->location,
                op_conversion,
                t_t::void_type{}
            );
            body.emplace_back(std::make_unique<ast_node>(std::move(drop_wrapper)));
        }
    }

    if (!capy_lexer.ahead_is_sym(token_symbol::sym_curly_close))
    {
        append_error("Expecting a closing brace '}' to terminate the body");
    }
}

size_t parser::collect_literal(const std::string& literal)
{
    size_t insert_index = current_module->string_literals.size();
    current_module->string_literals.emplace_back(
        node_module::string_literal_entry{0, literal});

    return insert_index;
}
