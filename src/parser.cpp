#include "parser.hpp"
#include <assert.h>
#include <iostream>
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

void dump_node(const node_number& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');

    std::cout << ind << "Num:" << n.number << "\n";
}

void dump_node(const node_string_literal& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');

    std::cout << ind << "String literal: \"" << n.string_literal<< "\"\n";
}

void dump_node(const node_var_reference& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');

    std::cout << ind << "Var(" << repr_type(n.symbol_ref.symbol_type) << "):" << n.name << "\n";
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
    dump_ast(*n.function_head, indent);
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

void dump_node(const node_module& n, size_t indent)
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

void dump_node(const node_parse_error& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');

    std::cout << ind << "PARSE ERROR:"+n.error_message+"'\n";
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

t_t::pointer::pointer(const type_kind& base_type)
: base_type(make_unique<type_kind>(base_type))
{
}

t_t::pointer::pointer(const t_t::pointer& other)
: base_type(make_unique<type_kind>(*other.base_type))
{
}

t_t::pointer& t_t::pointer::operator=(const t_t::pointer& other)
{
    base_type = make_unique<type_kind>(*other.base_type);
    return *this;
}

bool t_t::pointer::operator==(const pointer& other) const
{
    return *base_type == *other.base_type;
}

t_t::record::field_spec::field_spec(const std::string& name, std::unique_ptr<type_kind> type_spec)
: name(name)
, type_spec(std::make_unique<type_kind>(*type_spec))
{
}

t_t::record::field_spec::field_spec(const t_t::record::field_spec& other)
: name(other.name)
, type_spec(std::make_unique<type_kind>(*other.type_spec))
{
}

t_t::record::field_spec& t_t::record::field_spec::operator=(const t_t::record::field_spec& other)
{
    name = other.name;
    type_spec = make_unique<type_kind>(*other.type_spec);
    return *this;
}

t_t::record::record(const std::vector<t_t::record::field_spec>& fields)
: fields(fields)
{
}

t_t::record::record(const t_t::record& other)
: fields(other.fields)
{
}

t_t::record& t_t::record::operator=(const record& other)
{
    fields = other.fields;
    return *this;
}

bool t_t::record::operator==(const record& other) const
{
    return true;
}

std::optional<type_kind> t_t::record::field_type(const std::string& name)
{
    for (const auto& field : fields)
    {
        if (field.name == name)
        {
            return *field.type_spec;
        }
    }
    return std::nullopt;
}

std::string repr_type(const type_kind& type_spec)
{
    return std::visit([&](const auto &t) -> std::string {
        using T = std::decay_t<decltype(t)>;

        if constexpr (std::is_same_v<T, t_t::unassigned>)
            return "!unassigned";
        else if constexpr (std::is_same_v<T, t_t::void_type>)
            return "void";
        else if constexpr (std::is_same_v<T, t_t::s32>)
            return "s32";
        else if constexpr (std::is_same_v<T, t_t::u8>)
            return "u8";
        else if constexpr (std::is_same_v<T, t_t::u32>)
            return "u32";
        else if constexpr (std::is_same_v<T, t_t::pointer>) {
            if (!t.base_type) {
                std::cout << "   Uh oh, null base_type in pointer\n";
                return "*null*";
            }
            return repr_type(*t.base_type) + "*";
        }
        else if constexpr (std::is_same_v<T, t_t::record>) {
            std::string repr = "record(";
            for (const auto& field: t.fields) {
                repr += field.name + ":";
                repr += repr_type(*field.type_spec) + ",";
            }
            repr += ")";
            return repr;
        }
        else {
            return "UNEXPECTED BRANCH";
        }
    }, type_spec);
}

bool function_signature::equals_call_signature(function_signature &other)
{
    if (parameters.size() != other.parameters.size())
    {
        return false;
    }

    for (size_t param_index = 0; param_index < parameters.size(); param_index++)
    {
        if (parameters[param_index].type_spec != other.parameters[param_index].type_spec)
        {
            return false;
        }
    }

    return true;
}

std::string function_signature::repr()
{
    std::string r = "(";
    if (parameters.size()>0)
    {
        r += repr_type(parameters[0].type_spec);

        size_t index = 1;
        while (index<parameters.size())
        {
            r += ", " + repr_type(parameters[index].type_spec);
            index++;
        }
    }
    r += ")";
    return r;
}

scope* scope::get_global_scope() const
{
    scope* scope_iter = const_cast<scope*>(this);
    while (scope_iter->parent != nullptr)
    {
        scope_iter = scope_iter->parent;
    }
    return scope_iter;
}

std::optional<symbol> scope::lookup(const std::string &name)
{
    if (symbol_table.find(name) != symbol_table.end())
    {
        return symbol_table[name];
    }
    else if (parent != nullptr)
    {
        return parent->lookup(name);
    }
    else
    {
        return std::nullopt;
    }
}

std::optional<type_kind> scope::lookup_type(const std::string& name)
{
    if (type_table.find(name) != type_table.end())
    {
        return type_table[name];
    }
    else if (parent != nullptr)
    {
        return parent->lookup_type(name);
    }
    else
    {
        return std::nullopt;
    }
}

std::optional<func_symbol> scope::lookup_function(const std::string &name)
{
    if (function_table.find(name) != function_table.end())
    {
        return function_table[name];
    }
    else if (parent != nullptr)
    {
        return parent->lookup_function(name);
    }
    else
    {
        return std::nullopt;
    }
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
    else if (id == "string")
    {
        return t_t::string{};
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
        append_error("Unexpected trailing code after function definition");
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

void parser::append_error(const std::string &error_message)
{
    auto current_pos = capy_lexer.current_source_position();

    errors.emplace_back(node_parse_error(
        current_pos,
        error_message));
}

void parser::append_error_at(source_position location, const std::string &error_message)
{
    errors.emplace_back(node_parse_error(
        location,
        error_message));
}

ast_node parser::parse_module()
{
    auto capy_module = make_located<node_module>(
        source_position{1, 1},
        source_position{1, 1});

    std::get<node_module>(capy_module.value).module_scope = std::make_unique<scope>();
    current_scope = std::get<node_module>(capy_module.value).module_scope.get();

    while (capy_lexer.ahead_is_sym(token_symbol::sym_kw_import))
    {
        auto import_def = parse_import_definition();

        std::get<node_module>(capy_module.value).imports.push_back(std::make_unique<ast_node>(std::move(import_def)));
    }

    while (capy_lexer.ahead_is_sym(token_symbol::sym_kw_record))
    {
        auto record_def = parse_record_definition();
        if (is_error(record_def))
        {
            return record_def;
        }

        std::get<node_module>(capy_module.value).typedefs.push_back(std::make_unique<ast_node>(std::move(record_def)));
    }

    while (capy_lexer.ahead_is_sym(token_symbol::sym_kw_fn))
    {
        auto function = parse_function_definition();

        std::get<node_module>(capy_module.value).functions.push_back(std::make_unique<ast_node>(std::move(function)));
    }

    return capy_module;
}

void parser::parse_function_signature(function_signature &signature)
{
    if (!capy_lexer.expect_symbol(token_symbol::sym_brac_open))
    {
        append_error("Expecting an opening bracket '(' for function parameters");
    }

    parse_parameters(signature.parameters);

    if (!capy_lexer.expect_symbol(token_symbol::sym_brac_close))
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
    uint32_t argument_index = 0;

    while (capy_lexer.ahead_is<token_identifier>())
    {
        auto param_name = capy_lexer.expect<token_identifier>();
        if (!capy_lexer.expect_symbol(token_symbol::sym_colon))
        {
            append_error("Expecting a colon ':' between parameter name and parameter type");
        }

        auto type_spec_node = parse_type_reference();
        auto type_spec = std::get<node_type_spec>(type_spec_node.value).type_spec;

        current_scope->symbol_table[param_name.name] = symbol{
            .name = param_name.name,
            .symbol_type = type_spec,
            .kind = symbol_kind::argument,
            .index_addr = argument_index++};

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
        std::make_unique<ast_node>(std::move(function_head)),
        alias_name);
}

ast_node parser::parse_function_definition()
{
    auto start_range = capy_lexer.expect<token_symbol>().location;

    auto function_head = parse_function_head();

    if (!capy_lexer.expect_symbol(token_symbol::sym_curly_open))
    {
        append_error("Expecting an opening brace '{' for function body definition");
    }

    auto func_scope = std::make_unique<scope>();
    func_scope->parent = current_scope;
    current_scope = func_scope.get();

    std::vector<std::unique_ptr<ast_node>> function_body;
    auto expression = parse_expression();
    function_body.emplace_back(std::make_unique<ast_node>(std::move(expression)));

    while (capy_lexer.ahead_is_sym(token_symbol::sym_semicolon))
    {
        capy_lexer.expect_symbol(token_symbol::sym_semicolon);

        // If the previous expression wasn't the last one in the function, then it
        // will have left a value on the stack and will make the WASM validation
        // unhappy. So we have to "convert" the type of the last expression to 'void'
        // so the emitter can insert a 'drop' instruction
        auto previous_expression = std::move(function_body.back());
        function_body.pop_back();
    
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
        function_body.emplace_back(std::make_unique<ast_node>(std::move(drop_wrapper)));

        expression = parse_expression();
        function_body.emplace_back(std::make_unique<ast_node>(std::move(expression)));
    }

    if (!capy_lexer.ahead_is_sym(token_symbol::sym_curly_close))
    {
        append_error("Expecting a closing brace '}' after function body definition");
    }
    auto end_range = capy_lexer.expect<token_symbol>().location;

    current_scope = current_scope->parent;

    return make_located<node_function_definition>(
        start_range.start,
        end_range.end,
        std::make_unique<ast_node>(std::move(function_head)),
        std::move(function_body),
        std::move(func_scope));
}

ast_node parser::parse_function_head()
{
    if (!capy_lexer.ahead_is<token_identifier>())
    {
        append_error("Expecting a function name");
    }
    auto function_name = capy_lexer.parse_or_default<token_identifier>();
    auto start_range = function_name.location;

    function_signature signature;
    parse_function_signature(signature);

    current_scope->function_table[function_name.name] = func_symbol{
        .name = function_name.name,
        .signature = signature};

    return make_located<node_function_head>(
        start_range.start,
        start_range.end, // TODO: this should span the whole signature
        function_name.name,
        signature);
}

ast_node parser::parse_expression(int min_precedence)
{
    if (capy_lexer.ahead_is_sym(token_symbol::sym_brac_open))
    {
        // eat up the '(' token
        auto start = capy_lexer.expect<token_symbol>().location;

        auto expression = parse_expression();

        // check for a closing )
        if (capy_lexer.ahead_is_sym(token_symbol::sym_brac_close))
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
    while (!capy_lexer.ahead_is_sym(token_symbol::sym_brac_close))
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

        func_symbol dummy_symbol;
        return make_located<node_function_call>(
            name_range.start,
            end_location.end,
            function_name,
            dummy_symbol,
            std::move(function_parameters));
    }

    return make_located<node_function_call>(
        name_range.start,
        end_location.end,
        function_name,
        func.value(),
        std::move(function_parameters));
}

ast_node parser::parse_let_expression()
{
    // eat the 'let' keyword
    auto start_location = capy_lexer.expect<token_symbol>().location;

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

    if (!capy_lexer.expect_symbol(token_symbol::sym_equal)) 
    {
        append_error("Expecting a '=' starting the initializer expression for the new variable");
    }

    auto initializer = parse_expression();

    auto new_symbol = symbol{
        .name = variable_name.name,
        .symbol_type = type_spec,
        .kind = symbol_kind::local_var,
        .index_addr = current_scope->symbol_table.size(),
    };
    current_scope->symbol_table[variable_name.name] = new_symbol;

    return make_located<node_let_expression>(
        start_location.start,
        initializer.location.end,
        variable_name.name,
        type_spec,
        new_symbol,
        std::make_unique<ast_node>(std::move(initializer))
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
    global_scope->type_table[record_id.name] = t_t::record(new_record_fields);

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

        if (capy_lexer.ahead_is_sym(token_symbol::sym_brac_open))
        {
            return parse_function_call(id_range, id.name);
        }
        else if (capy_lexer.ahead_is_sym(token_symbol::sym_curly_open))
        {
            return parse_record_initialisation(id_range, id.name);
        }
        else if (capy_lexer.ahead_is_sym(token_symbol::sym_period))
        {
            auto var = current_scope->lookup(id.name);
            if (!var.has_value())
            {
                append_error_at(id_range.start, "Undefined variable: '"+id.name+"'");
                var = symbol{
                    "_",
                    t_t::u32{},
                    symbol_kind::global_var,
                    0
                };
            }

            ast_node object = make_located<node_var_reference>(
                id_range.start,
                id_range.end,
                id.name,
                var.value(),
                assign_context::rhs);
            auto var_type = var.value().symbol_type;

            return parse_field_deref(var_type, std::move(object));
        }
        else
        {
            auto var = current_scope->lookup(id.name);
            if (!var.has_value())
            {
                append_error_at(id_range.start, "Undefined variable: '"+id.name+"'");
                var = symbol{
                    "_",
                    t_t::u32{},
                    symbol_kind::global_var,
                    0
                };
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
        return make_located<node_string_literal>(
            literal_location.start,
            literal_location.end,
            literal.str);
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