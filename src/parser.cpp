#include "parser.hpp"
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

void dump_node(const node_struct_definition& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');

    std::cout << ind << "Struct definition " << n.name << ":\n";
    for (const auto& field : n.fields)
    {
        std::cout << ind << "  " << field.name << ": " << repr_type(*field.type_spec) << "\n";
    }
}

void dump_node(const node_struct_initialisation& n, size_t indent)
{
    std::string ind = std::string(indent, ' ');

    std::cout << ind << "Struct init\n";
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

    std::cout << ind << "PARSE ERROR\n";
}

void dump_ast(const ast_node& root, size_t indent)
{
    std::visit([=](const auto &n)
        {
            dump_node(n, indent);
        }, root.value);
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
            // TODO: add output of field types
            return "record";
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

    std::get<node_module>(capy_module.value).module_scope = std::make_unique<scope>();
    current_scope = std::get<node_module>(capy_module.value).module_scope.get();

    while (capy_lexer.ahead_is_sym(token_symbol::sym_kw_import))
    {
        auto import_def = parse_import_definition();
        if (is_error(import_def))
        {
            return import_def;
        }

        std::get<node_module>(capy_module.value).imports.push_back(std::make_unique<ast_node>(std::move(import_def)));
    }

    while (capy_lexer.ahead_is_sym(token_symbol::sym_kw_struct))
    {
        auto struct_def = parse_struct_definition();
        if (is_error(struct_def))
        {
            return struct_def;
        }

        std::get<node_module>(capy_module.value).typedefs.push_back(std::make_unique<ast_node>(std::move(struct_def)));
    }

    while (capy_lexer.ahead_is_sym(token_symbol::sym_kw_fn))
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

std::optional<ast_node> parser::parse_function_signature(function_signature &signature)
{
    // TODO: we lost how we can keep to location of the signature here :-(
    if (!capy_lexer.expect_symbol(token_symbol::sym_brac_open))
    {
        return create_error("Expecting an opening bracket '(' for function parameters");
    }

    auto error_result = parse_parameters(signature.parameters);
    if (error_result.has_value())
    {
        return std::move(error_result.value());
    }

    if (!capy_lexer.expect_symbol(token_symbol::sym_brac_close))
    {
        return create_error("Expecting an closing bracket ')' for function parameters");
    }

    std::optional<type_kind> return_type = t_t::void_type{};

    if (capy_lexer.ahead_is_sym(token_symbol::sym_arrow))
    {
        capy_lexer.next_token();

        auto type_spec_node = parse_type_reference();
        if (is_error(type_spec_node))
        {
            return type_spec_node;
        }
        return_type = std::get<node_type_spec>(type_spec_node.value).type_spec;
    }

    signature.return_type = return_type.value();

    return std::nullopt;
}

std::optional<ast_node> parser::parse_parameters(std::vector<param_spec> &parameters)
{
    uint32_t argument_index = 0;

    while (capy_lexer.ahead_is<token_identifier>())
    {
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

    auto function_head = parse_function_head();
    if (is_error(function_head))
    {
        return function_head;
    }

    std::optional<std::string> alias_name;

    if (capy_lexer.ahead_is_sym(token_symbol::sym_kw_as))
    {
        capy_lexer.next_token();

        if (!capy_lexer.ahead_is<token_identifier>())
        {
            return create_error("Expecting an alias identifier for imported symbol");
        }
        auto [_, alias_name] = capy_lexer.expect<token_identifier>();
    }

    if (!capy_lexer.ahead_is_sym(token_symbol::sym_semicolon))
    {
        return create_error("Expecting a closing ';' after import definition");
    }
    auto [end_range, _] = capy_lexer.expect<token_symbol>();

    return make_located<node_import_definition>(
        start_range.start,
        end_range.end,
        namespace_name.name,
        std::make_unique<ast_node>(std::move(function_head)),
        alias_name);
}

ast_node parser::parse_function_definition()
{
    auto [start_range, _] = capy_lexer.expect<token_symbol>();

    auto function_head = parse_function_head();
    if (is_error(function_head))
    {
        return function_head;
    }

    if (!capy_lexer.expect_symbol(token_symbol::sym_curly_open))
    {
        return create_error("Expecting an opening brace '{' for function body definition");
    }

    auto func_scope = std::make_unique<scope>();
    func_scope->parent = current_scope;
    current_scope = func_scope.get();

    std::vector<std::unique_ptr<ast_node>> function_body;
    auto expression = parse_expression();
    if (is_error(expression))
    {
        return expression;
    }
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
        if (is_error(expression))
        {
            return expression;
        }
        function_body.emplace_back(std::make_unique<ast_node>(std::move(expression)));
    }

    if (!capy_lexer.ahead_is_sym(token_symbol::sym_curly_close))
    {
        return create_error("Expecting a closing brace '}' after function body definition");
    }
    auto [end_range, _] = capy_lexer.expect<token_symbol>();

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
        return create_error("Expecting a function name");
    }
    auto [start_range, function_name] = capy_lexer.expect<token_identifier>();

    function_signature signature;
    auto signature_result = parse_function_signature(signature);
    if (signature_result.has_value())
    {
        return std::move(signature_result.value());
    }

    current_scope->function_table[function_name.name] = func_symbol{
        .name = function_name.name,
        .signature = signature};

    return make_located<node_function_head>(
        start_range.start,
        start_range.end, // TODO
        function_name.name,
        signature);
}

ast_node parser::parse_expression(int min_precedence)
{
    if (capy_lexer.ahead_is_sym(token_symbol::sym_brac_open))
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
        if (capy_lexer.ahead_is_sym(token_symbol::sym_brac_close))
        {
            auto [end, _] = capy_lexer.expect<token_symbol>();

            if (capy_lexer.ahead_is_sym(token_symbol::sym_kw_as))
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
                    op_conversion,
                    t_t::unassigned{}
                );
            }
            else
            {
                return expression;
            }
        }

        return create_error("Expected a closing brace ')' at the end of the expression");
    }
    else if (capy_lexer.ahead_is_sym(token_symbol::sym_kw_let))
    {
        return parse_let_expression();
    }
    else
    {
        auto lhs = parse_primary();
        if (is_error(lhs))
        {
            return lhs;
        }
        source_range start = lhs.location;

        while (capy_lexer.ahead_is_operator())
        {
            auto op = op_from_symbol(capy_lexer.next_as<token_symbol>());
            int prec = get_precedence(op);
            if (prec < min_precedence)
                break;
            auto op_token = capy_lexer.next_token();

            ast_node rhs;
            if (op == op_conversion)
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
        if (is_error(parameter))
        {
            return parameter;
        }
        function_parameters.emplace_back(std::make_unique<ast_node>(std::move(parameter)));

        if (capy_lexer.ahead_is_sym(token_symbol::sym_comma))
        {
            // eat the separating ','
            capy_lexer.expect_symbol(token_symbol::sym_comma);
        }
    }

    // TODO: check for closing )
    auto [end_location, _] = capy_lexer.expect<token_symbol>();

    auto func = current_scope->lookup_function(function_name);
    if (!func.has_value())
    {
        return create_error("Function '" + function_name + "' is not defined");
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
    auto [start_location, _] = capy_lexer.expect<token_symbol>();

    if (!capy_lexer.ahead_is<token_identifier>())
    {
        return create_error("Expecting a variable name after 'let' keyword");
    }
    auto [_, variable_name] = capy_lexer.expect<token_identifier>();

    if (!capy_lexer.expect_symbol(token_symbol::sym_colon))
    {
        return create_error("Expecting a ':' after variable name in 'let' expression");
    }

    auto type_spec_node = parse_type_reference();
    if (is_error(type_spec_node))
    {
        return type_spec_node;
    }
    auto type_spec = std::get<node_type_spec>(type_spec_node.value).type_spec;

    if (!capy_lexer.expect_symbol(token_symbol::sym_equal)) 
    {
        return create_error("Expecting a '=' starting the initializer expression for the new variable");
    }

    auto initializer = parse_expression();
    if (is_error(initializer))
    {
        return initializer;
    }

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

ast_node parser::parse_struct_definition()
{
    // eat the 'struct' keyword
    auto [start_range, _] = capy_lexer.expect<token_symbol>();

    if (!capy_lexer.ahead_is<token_identifier>())
    {
        return create_error("'struct' definition requires an identifier for the struct");
    }

    auto  [_, struct_id] = capy_lexer.expect<token_identifier>();

    if (!capy_lexer.expect_symbol(token_symbol::sym_curly_open))
    {
        return create_error("Expecting an opening brace '{' starting the struct definition");
    }
    
    std::vector<t_t::record::field_spec> new_struct_fields;
    while (capy_lexer.ahead_is<token_identifier>())
    {
        auto [_, field_id] = capy_lexer.expect<token_identifier>();
        if (!capy_lexer.expect_symbol(token_symbol::sym_colon))
        {
            return create_error("Expecting a colon ':' after field name and before type specification");
        }

        auto type_spec_node = parse_type_reference();
        if (is_error(type_spec_node))
        {
            return type_spec_node;
        }
        auto type_spec = std::get<node_type_spec>(type_spec_node.value).type_spec;

        new_struct_fields.emplace_back(t_t::record::field_spec{field_id.name, std::make_unique<type_kind>(type_spec)});

        if (!capy_lexer.expect_symbol(token_symbol::sym_comma))
        {
            return create_error("Struct field definitions must be terminated with a ','");
        }
    }

    if (!capy_lexer.ahead_is_sym(token_symbol::sym_curly_close))
    {
        return create_error("Struct definition must be closed with a matching '}'");
    }
    capy_lexer.expect<token_symbol>();
    if (!capy_lexer.ahead_is_sym(token_symbol::sym_semicolon))
    {
        return create_error("Struct definition must be terminated with a semicolon");
    }
    auto [end_range, _] = capy_lexer.expect<token_symbol>();

    auto* global_scope = current_scope->get_global_scope();
    global_scope->type_table[struct_id.name] = t_t::record(new_struct_fields);

    return make_located<node_struct_definition>(
        start_range.start,
        end_range.end,
        struct_id.name,
        new_struct_fields
    );
}

ast_node parser::parse_struct_initialisation(source_range name_range, const std::string& struct_name)
{
    auto struct_type = current_scope->lookup_type(struct_name);
    if (!struct_type.has_value())
    {
        return create_error("Trying to initialise struct of unknown type '"+struct_name+"'");
    }

    // skipping the opening '{'
    capy_lexer.expect<token_symbol>();

    std::vector<field_initialisation> fields;

    while (capy_lexer.ahead_is<token_identifier>())
    {
        auto [_, field_name] = capy_lexer.expect<token_identifier>();

        if (!capy_lexer.ahead_is_sym(token_symbol::sym_equal))
        {
            return create_error("For field initialisation, field name must be followed by a '=' and an initialisation expression");
        }
        capy_lexer.expect<token_symbol>();

        ast_node init_expression = parse_expression();

        if (!capy_lexer.ahead_is_sym(token_symbol::sym_comma))
        {
            return create_error("Field initialisations must be terminated by a comma");
        }
        capy_lexer.expect<token_symbol>();
        
        fields.emplace_back(field_initialisation{field_name.name, std::make_unique<ast_node>(std::move(init_expression))});
    }

    if (!capy_lexer.ahead_is_sym(token_symbol::sym_curly_close))
    {
        return create_error("Struct initialisation must be finished with a closing '}'");
    }
    auto [end_range, _] = capy_lexer.expect<token_symbol>();

    return make_located<node_struct_initialisation>(
        name_range.start,
        end_range.end,
        struct_type.value(),
        std::move(fields)
    );
}

ast_node parser::parse_field_deref(type_kind base_type, ast_node object)
{
    // eat the dot '.'
    capy_lexer.expect<token_symbol>();

    // TODO: still have to check that there actually is an identifier after the dot
    auto [field_range, field_name] = capy_lexer.expect<token_identifier>();

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
            return create_error("Can't dereference field '"+field_name.name+"'");
        }
        t_t::record base_record_type = std::get<t_t::record>(base_type);
        auto field_type = base_record_type.field_type(field_name.name);
        if (!field_type.has_value())
        {
            return create_error("Record doesn't contain a field named '"+field_name.name+"'");
        }

        node = parse_field_deref(field_type.value(), std::move(node));
    }

    return node;
}

ast_node parser::parse_primary()
{
    if (capy_lexer.ahead_is<token_identifier>())
    {
        auto [id_range, id] = capy_lexer.expect<token_identifier>();

        if (capy_lexer.ahead_is_sym(token_symbol::sym_brac_open))
        {
            return parse_function_call(id_range, id.name);
        }
        else if (capy_lexer.ahead_is_sym(token_symbol::sym_curly_open))
        {
            return parse_struct_initialisation(id_range, id.name);
        }
        else if (capy_lexer.ahead_is_sym(token_symbol::sym_period))
        {
            auto var = current_scope->lookup(id.name);
            if (!var.has_value())
            {
                return create_error("Undefined variable: '"+id.name+"'");
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
                return create_error("Undefined variable: '"+id.name+"'");
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
        auto op_token = capy_lexer.next_token();

        auto pointer_expr = parse_primary();
        if (is_error(pointer_expr))
        {
            return pointer_expr;
        }

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
    else
    {
        return create_error("Expected a primary (function call, number, variable)");
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

    if (!capy_lexer.ahead_is<token_identifier>())
    {
        return create_error("Expecting an identifier for the type specification");
    }
    auto [token_location, type_name] = capy_lexer.expect<token_identifier>();

    auto type_spec = type_from_id(type_name.name);
    if (!type_spec.has_value())
    {
        type_spec = current_scope->lookup_type(type_name.name);
        if (!type_spec.has_value())
        {
            return create_error("Unknown type specification: " + type_name.name);
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