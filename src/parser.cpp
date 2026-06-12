#include "parser.hpp"
#include "diagnostics.hpp"
#include "tools.hpp"
#include <format>
#include <limits>
#include <memory>
#include <ranges>
#include <sstream>
#include <string>

template <typename T, typename... Args>
node_expr make_located(source_position start, source_position end, Args&&... args)
{
    return node_expr{
        .value = T{{}, std::forward<Args>(args)...},
        .location = source_range{
            .start = start,
            .end = end
        }
    };
}

bool bool_from_string(std::string str)
{
    return (str == "true");
}

std::optional<type_id> type_from_id(context& ctx, const std::string& id)
{
    if (id == "u32")
    {
        return ctx.intern_primitive(primitive_type::U32);
    }
    else if (id == "u16")
    {
        return ctx.intern_primitive(primitive_type::U16);
    }
    else if (id == "u8")
    {
        return ctx.intern_primitive(primitive_type::U8);
    }
    else if ((id == "s32"))
    {
        return ctx.intern_primitive(primitive_type::S32);
    }
    else if (id == "s16")
    {
        return ctx.intern_primitive(primitive_type::S16);
    }
    else if (id == "s8")
    {
        return ctx.intern_primitive(primitive_type::S8);
    }
    else if (id == "char")
    {
        return ctx.intern_primitive(primitive_type::Char);
    }
    else if (id == "string")
    {
        return ctx.intern_primitive(primitive_type::String);
    }
    else if (id == "bool")
    {
        return ctx.intern_primitive(primitive_type::Boolean);
    }

    return std::nullopt;
}

parser::parser(diagnostic_bag& diagnostics, std::shared_ptr<lexer> l, context& ctx)
: diagnostic_emitter(diagnostics, diagnostic_phase::parser)
, capy_lexer(l)
, parse_context(ctx)
, error_symbol(ILLEGAL_SYMBOL)
{
    error_symbol = parse_context.create_symbol(symbol{
        .name = "_error",
        .location = source_range{.start = source_position{__FILE__, __LINE__, 0}, .end = source_position{__FILE__, __LINE__, 0}},
        .symbol_type = ILLEGAL_TYPE,
        .signature = function_signature{},
        .kind = symbol_kind::error,
        .mutab = false,
        .is_assigned = true,
        .is_intrinsic = false,
    });
}

node_module parser::parse()
{
    auto root = parse_module();
    if (!capy_lexer->ahead_is<token_eof>())
    {
        append_error("Unexpected trailing code after function definition");
    }
    return root;
}

void parser::append_error(const std::string& error_message)
{
    auto current_pos = capy_lexer->current_source_position();
    append_error_at(current_pos, error_message);
}

void parser::populate_intrinsics()
{
    function_type mem_size_type;
    mem_size_type.return_type = parse_context.intern_primitive(primitive_type::U32);
    function_signature mem_size_sig;
    mem_size_sig.function_type = parse_context.intern(mem_size_type);
    current_scope->symbol_table["memory_size"] =
        parse_context.create_symbol(symbol{
            .name = "memory_size",
            .location = source_range{.start = source_position{__FILE__, __LINE__, 0}, .end = source_position{__FILE__, __LINE__, 0}},
            .symbol_type = mem_size_sig.function_type,
            .signature = mem_size_sig,
            .kind = symbol_kind::function,
            .mutab = false,
            .is_assigned = true,
            .is_intrinsic = true,
        });

    function_type mem_grow_type;
    mem_grow_type.parameter_types.push_back(parse_context.intern_primitive(primitive_type::U32));
    mem_grow_type.return_type = parse_context.intern_primitive(primitive_type::S32);
    function_signature mem_grow_sig;
    mem_grow_sig.parameters.push_back(function_parameter{source_position{__FILE__, __LINE__, 0}, "additional_blocks"});
    mem_grow_sig.function_type = parse_context.intern(mem_grow_type);
    current_scope->symbol_table["memory_grow"] =
        parse_context.create_symbol(symbol{
            .name = "memory_grow",
            .location = source_range{.start = source_position{__FILE__, __LINE__, 0}, .end = source_position{__FILE__, __LINE__, 0}},
            .symbol_type = mem_grow_sig.function_type,
            .signature = mem_grow_sig,
            .kind = symbol_kind::function,
            .mutab = false,
            .is_assigned = true,
            .is_intrinsic = true,
        });

    function_type unreachable_type;
    unreachable_type.return_type = parse_context.intern_primitive(primitive_type::Void);
    function_signature unreachable_sig;
    unreachable_sig.function_type = parse_context.intern(unreachable_type);
    current_scope->symbol_table["unreachable"] =
        parse_context.create_symbol(symbol{
            .name = "unreachable",
            .location = source_range{.start = source_position{__FILE__, __LINE__, 0}, .end = source_position{__FILE__, __LINE__, 0}},
            .symbol_type = unreachable_sig.function_type,
            .signature = unreachable_sig,
            .kind = symbol_kind::function,
            .mutab = false,
            .is_assigned = true,
            .is_intrinsic = true,
        });
}

node_module parser::parse_module()
{
    auto capy_module = node_module{
        {{},
         {source_position{"", 1, 1}, source_position{"", 1, 1}}}
    };

    current_module = &capy_module;

    capy_module.module_scope = std::make_unique<scope>();
    current_scope = capy_module.module_scope.get();
    populate_intrinsics();

    std::string library_code = R"(
global mut heap_ptr: u32 = 1024u32;

fn cabi_realloc(originalPtr: u32, originalSize: u32, alignment: u32, newSize: u32) -> u32
{
    let alloc_address: u32 = heap_ptr + (alignment - (heap_ptr%alignment));
    heap_ptr = alloc_address + newSize;

    // determine the amount of required pages for the new heap size
    let needed_pages = (heap_ptr / 65536u32) + 1u32;
    if (needed_pages > memory_size()) {
        memory_grow(needed_pages - memory_size());
    }

    alloc_address
}
)";
    std::istringstream lib_stream{library_code};
    auto previous_lexer = capy_lexer;
    diagnostic_bag diagnostics;
    capy_lexer = std::make_shared<lexer>(diagnostics, lib_stream, "stdlib");
    parse_module_body();
    capy_lexer = previous_lexer;

    parse_module_body();

    current_module = nullptr;
    return capy_module;
}

void parser::parse_module_body()
{
    while (capy_lexer->ahead_is_sym(token_symbol::sym_at) ||
           capy_lexer->ahead_is_sym(token_symbol::sym_kw_import) ||
           capy_lexer->ahead_is_sym(token_symbol::sym_kw_global) ||
           capy_lexer->ahead_is_sym(token_symbol::sym_kw_record) ||
           capy_lexer->ahead_is_sym(token_symbol::sym_kw_fn))
    {
        if (capy_lexer->ahead_is_sym(token_symbol::sym_at))
        {
            parse_attribute();
        }
        else if (capy_lexer->ahead_is_sym(token_symbol::sym_kw_import))
        {
            auto import_def = parse_import_definition();

            current_module->imports.push_back(std::make_unique<node_import_definition>(std::move(import_def)));
        }
        else if (capy_lexer->ahead_is_sym(token_symbol::sym_kw_global))
        {
            auto global_def = parse_global();
            current_module->globals.push_back(std::make_unique<node_global_definition>(std::move(global_def)));
        }
        else if (capy_lexer->ahead_is_sym(token_symbol::sym_kw_record))
        {
            auto record_def = parse_record_definition();
            current_module->typedefs.push_back(std::make_unique<node_expr>(std::move(record_def)));
        }
        else if (capy_lexer->ahead_is_sym(token_symbol::sym_kw_fn))
        {
            auto function = parse_function_definition();

            current_module->functions.push_back(std::make_unique<node_function_definition>(std::move(function)));
        }
    }
}

void parser::parse_attribute()
{
    capy_lexer->expect_symbol(token_symbol::sym_at);
    auto attribute_name = capy_lexer->expect<token_identifier>();
    collected_attributes.push_back(capy_attribute{attribute_name.name});

    if (capy_lexer->ahead_is_sym(token_symbol::sym_paren_open))
    {
        capy_lexer->expect_symbol(token_symbol::sym_paren_open);
        while (!capy_lexer->ahead_is_sym(token_symbol::sym_paren_close))
        {
            capy_lexer->expect<token_identifier>();
            capy_lexer->expect_symbol(token_symbol::sym_equal);
            parse_primary();

            if (capy_lexer->ahead_is_sym(token_symbol::sym_comma))
            {
                capy_lexer->expect_symbol(token_symbol::sym_comma);
            }
        }
        capy_lexer->expect_symbol(token_symbol::sym_paren_close);
    }
}

void parser::parse_function_signature(function_signature& signature)
{
    if (!capy_lexer->expect_symbol(token_symbol::sym_paren_open))
    {
        append_error("Expecting an opening bracket '(' for function parameters");
    }

    function_type new_func_type;
    parse_parameters(signature, new_func_type);

    if (!capy_lexer->expect_symbol(token_symbol::sym_paren_close))
    {
        append_error("Expecting an closing bracket ')' for function parameters");
    }

    type_id return_type = parse_context.intern_primitive(primitive_type::Void);

    if (capy_lexer->ahead_is_sym(token_symbol::sym_arrow))
    {
        capy_lexer->next_token();
        return_type = parse_type_reference();
    }

    new_func_type.return_type = return_type;
    // TODO: this whole area is very error prone; we need to create a new
    // function_type but we also must make sure that this type is interned to
    // get a new type_id and then we also have to assign this to the signature;
    // also we have to make sure, that we have also determined and assigned the
    // return type before interning the function type
    signature.function_type = parse_context.intern(new_func_type);
}

void parser::parse_parameters(function_signature& signature, function_type& func_type)
{
    while (capy_lexer->ahead_is<token_identifier>())
    {
        auto param_name = capy_lexer->expect<token_identifier>();
        signature.parameters.emplace_back(function_parameter{param_name.location, param_name.name});

        if (!capy_lexer->expect_symbol(token_symbol::sym_colon))
        {
            append_error("Expecting a colon ':' between parameter name and parameter type");
        }

        auto type_spec = parse_type_reference();
        func_type.parameter_types.emplace_back(type_spec);

        // eat the comma between the parameters
        if (capy_lexer->ahead_is_sym(token_symbol::sym_comma))
        {
            capy_lexer->expect<token_symbol>();
        }
    }
}

node_import_definition parser::parse_import_definition()
{
    // Eat the 'import' keyword that we already established in
    // the calling function
    auto start_range = capy_lexer->expect<token_symbol>().location;

    if (!capy_lexer->ahead_is<token_identifier>())
    {
        append_error("Expecting a namespace identifier");
    }
    auto namespace_name = capy_lexer->expect<token_identifier>();

    if (!capy_lexer->expect_symbol(token_symbol::sym_dcolon))
    {
        append_error("Namespace is expected to be followed by '::' and a symbol name");
    }

    auto function_head = parse_function_head();

    std::optional<std::string> alias_name;

    if (capy_lexer->ahead_is_sym(token_symbol::sym_kw_as))
    {
        capy_lexer->next_token();

        if (!capy_lexer->ahead_is<token_identifier>())
        {
            append_error("Expecting an alias identifier for imported symbol");
        }
        auto alias_token = capy_lexer->expect<token_identifier>();
        alias_name = alias_token.name;

        auto function_alias_symbol_id = parse_context.create_symbol(symbol{
            .name = alias_token.name,
            .location = alias_token.location,
            .symbol_type = function_head.signature.function_type,
            .signature = function_head.signature,
            .kind = symbol_kind::function,
            .mutab = false,
            .is_assigned = true,
            .is_intrinsic = false,
        });
        current_scope->symbol_table[alias_token.name] = function_alias_symbol_id;
    }

    if (!capy_lexer->ahead_is_sym(token_symbol::sym_semicolon))
    {
        append_error("Expecting a closing ';' after import definition");
    }
    auto end_range = capy_lexer->expect<token_symbol>().location;

    return node_import_definition{
        {{},
         {start_range.start, end_range.end}},
        namespace_name.name,
        std::make_unique<node_function_head>(std::move(function_head)),
        alias_name
    };
}

node_global_definition parser::parse_global()
{
    // eat the 'global' keyword
    auto start_location = capy_lexer->expect<token_symbol>().location;

    bool is_mutable = false;
    if (capy_lexer->ahead_is_sym(token_symbol::sym_kw_mut))
    {
        // eat the 'mut' keyword
        capy_lexer->expect<token_symbol>();
        is_mutable = true;
    }

    if (!capy_lexer->ahead_is<token_identifier>())
    {
        append_error("Expecting a variable name after 'global' keyword");
    }
    auto variable_name = capy_lexer->parse_or_default<token_identifier>();

    if (!capy_lexer->expect_symbol(token_symbol::sym_colon))
    {
        append_error("Expecting a ':' after variable name in 'global' expression");
    }

    auto type_spec = parse_type_reference();

    auto global_symbol = symbol{
        .name = variable_name.name,
        .location = variable_name.location,
        .symbol_type = type_spec,
        .kind = symbol_kind::global_var,
        .mutab = is_mutable,
        .is_assigned = false,
        .is_intrinsic = false,
    };
    auto global_symbol_id = parse_context.create_symbol(std::move(global_symbol));
    current_scope->symbol_table[variable_name.name] = global_symbol_id;

    int32_t init_value = 0;
    if (!capy_lexer->ahead_is_sym(token_symbol::sym_equal))
    {
        append_error("Globals need an initialisation value");
    }
    else
    {
        capy_lexer->expect_symbol(token_symbol::sym_equal);

        auto initializer = parse_number();
        init_value = std::get<node_number>(initializer.value).number;
        parse_context.symbol_at(global_symbol_id).is_assigned = true;
    }

    // eat the final semicolon
    auto end_token = capy_lexer->expect<token_symbol>();

    return node_global_definition{
        {{},
         {start_location.start, end_token.location.end}},
        variable_name.name,
        type_spec,
        global_symbol_id,
        init_value
    };
}

node_function_definition parser::parse_function_definition()
{
    auto attributes = collected_attributes;
    collected_attributes.clear();

    auto start_token = capy_lexer->expect<token_symbol>();
    auto start_range = start_token.location;

    auto function_head = parse_function_head();

    if (!capy_lexer->expect_symbol(token_symbol::sym_curly_open))
    {
        append_error("Expecting an opening brace '{' for function body definition");
    }

    auto func_scope = std::make_unique<scope>();
    func_scope->parent = current_scope;
    current_scope = func_scope.get();

    auto function_type_entry = parse_context.types[to_index(function_head.signature.function_type)];
    auto* func_type = get_type_from_node<function_type>(function_type_entry);
    CAPY_ASSERT(func_type != nullptr, "Compiler error");

    const auto& parameters = function_head.signature.parameters;
    for (auto [param, param_typ] : std::views::zip(parameters, func_type->parameter_types))
    {
        auto param_symbol_id = parse_context.create_symbol(symbol{
            .name = param.name,
            .location = param.location,
            .symbol_type = param_typ,
            .kind = symbol_kind::argument,
            .mutab = false,
            .is_assigned = true, // function parameters are 'assigned' from the function call
            .is_intrinsic = false,
        });
        current_scope->symbol_table[param.name] = param_symbol_id;
    }

    std::vector<std::unique_ptr<node_expr>> function_body;
    parse_body(function_body);
    auto end_range = capy_lexer->expect<token_symbol>().location;

    current_scope = current_scope->parent;

    return node_function_definition{
        {{{}, {start_range.start, end_range.end}}, std::move(attributes)},
        std::make_unique<node_function_head>(std::move(function_head)),
        std::move(function_body),
        std::move(func_scope)
    };
}

node_function_head parser::parse_function_head()
{
    if (!capy_lexer->ahead_is<token_identifier>())
    {
        append_error("Expecting a function name");
    }
    auto function_name = capy_lexer->parse_or_default<token_identifier>();
    auto start_range = function_name.location;

    function_signature signature;
    parse_function_signature(signature);

    auto function_symbol_id = parse_context.create_symbol(symbol{
        .name = function_name.name,
        .location = function_name.location,
        .symbol_type = signature.function_type,
        .signature = signature,
        .kind = symbol_kind::function,
        .mutab = false,
        .is_assigned = true,
        .is_intrinsic = false,
    });
    current_scope->symbol_table[function_name.name] = function_symbol_id;

    return node_function_head{
        {{},
         {start_range.start, start_range.end}}, // TODO: this should span the whole signature
        function_name.name,
        signature
    };
}

node_expr parser::parse_expression(int min_precedence)
{
    node_expr lhs;
    source_range start;

    if (capy_lexer->ahead_is_sym(token_symbol::sym_paren_open))
    {
        // eat up the '(' token
        auto start = capy_lexer->expect<token_symbol>().location;

        lhs = parse_expression();

        // check for a closing )
        if (capy_lexer->ahead_is_sym(token_symbol::sym_paren_close))
        {
            capy_lexer->expect<token_symbol>();
            start = lhs.location;
        }
        else
        {
            append_error("Expected a closing brace ')' at the end of the expression");

            // return a dummy expression so we can continue parsing
            return make_located<node_binary_expression>(
                start.start,
                start.end,
                std::unique_ptr<node_expr>(nullptr),
                std::unique_ptr<node_expr>(nullptr),
                source_range{start.start, start.end},
                op_multiply,
                parse_context.intern_primitive(primitive_type::U32)
            );
        }
    }
    else if (capy_lexer->ahead_is_sym(token_symbol::sym_kw_if))
    {
        lhs = parse_if_expression();
        start = lhs.location;
    }
    else if (capy_lexer->ahead_is_sym(token_symbol::sym_kw_while))
    {
        return parse_while_expression();
    }
    else if (capy_lexer->ahead_is_sym(token_symbol::sym_kw_let))
    {
        return parse_let_expression();
    }
    else if (capy_lexer->ahead_is_sym(token_symbol::sym_kw_return))
    {
        // eat the 'return' keyword
        auto start = capy_lexer->expect<token_symbol>().location;
        source_position end;
        std::unique_ptr<node_expr> returned_expression = nullptr;
        if (!capy_lexer->ahead_is_sym(token_symbol::sym_semicolon))
        {
            returned_expression = std::make_unique<node_expr>(parse_expression());
            end = returned_expression->location.end;
        }
        else
        {
            end = capy_lexer->current_source_position();
        }
        return make_located<node_return_expression>(
            start.start,
            end,
            std::move(returned_expression),
            true
        );
    }
    else if (capy_lexer->ahead_is_sym(token_symbol::sym_kw_break))
    {
        auto brk_statement = capy_lexer->expect<token_symbol>();
        return make_located<node_break_statement>(
            brk_statement.location.start,
            brk_statement.location.end
        );
    }
    else
    {
        lhs = parse_primary();
        start = lhs.location;
    }

    while (capy_lexer->ahead_is_operator())
    {
        auto op = op_from_symbol(capy_lexer->next_as<token_symbol>());
        int prec = get_precedence(op);
        if (prec < min_precedence)
            break;
        auto op_token = capy_lexer->expect<token_symbol>();

        node_expr rhs;
        type_id type_spec;
        source_range end = lhs.location;
        if (op == op_conversion)
        {
            type_spec = parse_type_reference();
            end.end = capy_lexer->current_source_position();
        }
        else
        {
            rhs = parse_expression(prec + 1);
            end = rhs.location;
        }

        if (op == op_conversion)
        {
            lhs = make_located<node_cast_expression>(
                start.start,
                end.end,
                std::make_unique<node_expr>(std::move(lhs)),
                type_spec,
                op_token.location
            );
        }
        else
        {
            lhs = make_located<node_binary_expression>(
                start.start,
                end.end,
                std::make_unique<node_expr>(std::move(lhs)),
                std::make_unique<node_expr>(std::move(rhs)),
                op_token.location,
                op,
                parse_context.create_type_var()
            );
        }
    }

    return lhs;
}

node_expr parser::parse_function_call(source_range name_range, const std::string function_name)
{
    // skip over the opening ( of the function call
    capy_lexer->expect<token_symbol>();

    std::vector<std::unique_ptr<node_expr>> function_parameters;
    while (!capy_lexer->ahead_is_sym(token_symbol::sym_paren_close))
    {
        auto parameter = parse_expression();
        function_parameters.emplace_back(std::make_unique<node_expr>(std::move(parameter)));

        if (capy_lexer->ahead_is_sym(token_symbol::sym_comma))
        {
            // eat the separating ','
            capy_lexer->expect_symbol(token_symbol::sym_comma);
        }
    }

    // TODO: check for closing )
    auto end_location = capy_lexer->expect<token_symbol>().location;

    auto maybe_symbol_id = current_scope->lookup(function_name);
    if ((!maybe_symbol_id.has_value()) ||
        (parse_context.symbol_at(maybe_symbol_id.value()).kind != symbol_kind::function))
    {
        append_error_at(name_range.start, "Function '" + function_name + "' is not defined");

        return make_located<node_function_call>(
            name_range.start,
            end_location.end,
            function_name,
            error_symbol,
            std::move(function_parameters)
        );
    }

    return make_located<node_function_call>(
        name_range.start,
        end_location.end,
        function_name,
        maybe_symbol_id.value(),
        std::move(function_parameters)
    );
}

node_expr parser::parse_if_expression()
{
    // eat the 'if' keyword
    auto start_location = capy_lexer->expect<token_symbol>().location;

    auto condition = parse_expression();

    if (!capy_lexer->expect_symbol(token_symbol::sym_curly_open))
    {
        append_error("Expecting an opening brace '{' for if-body");
    }

    std::vector<std::unique_ptr<node_expr>> then_body;
    std::vector<std::unique_ptr<node_expr>> else_body;
    parse_body(then_body);

    // eat the closing '}' of the then-branch
    auto end_range = capy_lexer->expect<token_symbol>().location;

    if (capy_lexer->ahead_is_sym(token_symbol::sym_kw_else))
    {
        // eat the 'else' keyword
        auto start_location = capy_lexer->expect<token_symbol>().location;

        if (!capy_lexer->expect_symbol(token_symbol::sym_curly_open))
        {
            append_error("Expecting an opening brace '{' for the else-body");
        }

        parse_body(else_body);

        end_range = capy_lexer->expect<token_symbol>().location;
    }

    return make_located<node_if_expression>(
        start_location.start,
        start_location.end,
        std::make_unique<node_expr>(std::move(condition)),
        std::move(then_body),
        std::move(else_body),
        parse_context.create_type_var()
    );
}

node_expr parser::parse_while_expression()
{
    // eat the 'while' keyword
    auto start_location = capy_lexer->expect<token_symbol>().location;

    auto condition = parse_expression();

    if (!capy_lexer->expect_symbol(token_symbol::sym_curly_open))
    {
        append_error("Expecting an opening brace '{' for while-body");
    }

    std::vector<std::unique_ptr<node_expr>> while_body;
    parse_body(while_body);

    // eat the closing '}' of the while block
    auto end_range = capy_lexer->expect<token_symbol>().location;

    return make_located<node_while_expression>(
        start_location.start,
        start_location.end,
        std::make_unique<node_expr>(std::move(condition)),
        std::move(while_body)
    );
}

node_expr parser::parse_let_expression()
{
    // eat the 'let' keyword
    auto start_location = capy_lexer->expect<token_symbol>().location;

    bool is_mutable = false;
    if (capy_lexer->ahead_is_sym(token_symbol::sym_kw_mut))
    {
        // eat the 'mut' keyword
        capy_lexer->expect<token_symbol>();
        is_mutable = true;
    }

    if (!capy_lexer->ahead_is<token_identifier>())
    {
        append_error("Expecting a variable name after 'let' keyword");
    }
    auto variable_name = capy_lexer->parse_or_default<token_identifier>();

    type_id type_spec;
    if (capy_lexer->ahead_is_sym(token_symbol::sym_colon))
    {
        capy_lexer->expect_symbol(token_symbol::sym_colon);
        type_spec = parse_type_reference();
    }
    else
    {
        type_spec = parse_context.create_type_var();
    }

    auto new_symbol = symbol{
        .name = variable_name.name,
        .location = variable_name.location,
        .symbol_type = type_spec,
        .kind = symbol_kind::local_var,
        .mutab = is_mutable,
        .is_assigned = false,
        .is_intrinsic = false,
    };
    auto local_symbol_id = parse_context.create_symbol(std::move(new_symbol));
    current_scope->symbol_table[variable_name.name] = local_symbol_id;

    if (capy_lexer->ahead_is_sym(token_symbol::sym_equal))
    {
        capy_lexer->expect_symbol(token_symbol::sym_equal);

        auto initializer = parse_expression();

        parse_context.symbol_at(local_symbol_id).is_assigned = true;
        return make_located<node_let_expression>(
            start_location.start,
            initializer.location.end,
            variable_name.name,
            local_symbol_id,
            std::make_unique<node_expr>(std::move(initializer))
        );
    }

    auto end_position = capy_lexer->current_source_position();
    return make_located<node_let_expression>(
        start_location.start,
        end_position,
        variable_name.name,
        local_symbol_id,
        nullptr
    );
}

node_expr parser::parse_record_definition()
{
    // eat the 'record' keyword
    auto start_range = capy_lexer->expect<token_symbol>().location;

    if (!capy_lexer->ahead_is<token_identifier>())
    {
        append_error("'record' definition requires an identifier for the record");
    }

    auto record_id = capy_lexer->expect<token_identifier>();

    if (!capy_lexer->expect_symbol(token_symbol::sym_curly_open))
    {
        append_error("Expecting an opening brace '{' starting the record definition");
    }

    std::vector<record_type::field_type> new_record_fields;

    while (capy_lexer->ahead_is<token_identifier>())
    {
        auto field_id = capy_lexer->expect<token_identifier>();
        if (!capy_lexer->expect_symbol(token_symbol::sym_colon))
        {
            append_error("Expecting a colon ':' after field name and before type specification");
        }

        auto type_spec = parse_type_reference();

        new_record_fields.emplace_back(record_type::field_type{field_id.name, type_spec});

        if (!capy_lexer->expect_symbol(token_symbol::sym_comma))
        {
            append_error("Record field definitions must be terminated with a ','");
        }
    }

    if (!capy_lexer->ahead_is_sym(token_symbol::sym_curly_close))
    {
        append_error("Record definition must be closed with a matching '}'");
    }
    capy_lexer->expect<token_symbol>();
    if (!capy_lexer->ahead_is_sym(token_symbol::sym_semicolon))
    {
        append_error("Record definition must be terminated with a semicolon");
    }
    auto end_range = capy_lexer->expect<token_symbol>().location;

    auto* global_scope = current_scope->get_global_scope();
    global_scope->type_table[record_id.name] = parse_context.intern(record_type{new_record_fields});

    return make_located<node_record_definition>(
        start_range.start,
        end_range.end,
        record_id.name,
        new_record_fields
    );
}

node_expr parser::parse_record_initialisation(source_range name_range, const std::string& record_name, bool is_allocated)
{
    auto rec_type = current_scope->lookup_type(record_name);
    if (!rec_type.has_value())
    {
        append_error("Trying to initialise record of unknown type '" + record_name + "'");
        rec_type = parse_context.intern_primitive(primitive_type::Void);
    }
    else if (is_allocated)
    {
        rec_type = parse_context.intern(pointer_type{rec_type.value()});
    }

    // skipping the opening '{'
    capy_lexer->expect<token_symbol>();

    std::vector<node_field_initialisation> fields;

    while (capy_lexer->ahead_is<token_identifier>())
    {
        auto field_name = capy_lexer->expect<token_identifier>();
        auto field_position = field_name.location;

        if (!capy_lexer->ahead_is_sym(token_symbol::sym_equal))
        {
            append_error("For field initialisation, field name must be followed by a '=' and an initialisation expression");
        }
        capy_lexer->expect<token_symbol>();

        node_expr init_expression = parse_expression();

        if (!capy_lexer->ahead_is_sym(token_symbol::sym_comma))
        {
            append_error("Field initialisations must be terminated by a comma");
        }
        capy_lexer->expect<token_symbol>();

        fields.emplace_back(node_field_initialisation{{}, field_position.start, field_name.name, std::make_unique<node_expr>(std::move(init_expression))});
    }

    if (!capy_lexer->ahead_is_sym(token_symbol::sym_curly_close))
    {
        append_error("Record initialisation must be finished with a closing '}'");
    }
    auto end_range = capy_lexer->expect<token_symbol>().location;

    return make_located<node_record_initialisation>(
        name_range.start,
        end_range.end,
        rec_type.value(),
        std::move(fields)
    );
}

node_expr parser::parse_field_deref(node_expr object)
{
    // eat the dot '.'
    capy_lexer->expect<token_symbol>();

    if (!capy_lexer->ahead_is<token_identifier>())
    {
        append_error("Missing field name after '.'");
        return make_located<node_field_deref>(
            object.location.start,
            object.location.end,
            std::make_unique<node_expr>(std::move(object)),
            "",
            ILLEGAL_TYPE
        );
    }
    auto field_name = capy_lexer->expect<token_identifier>();
    auto field_range = field_name.location;

    auto node = make_located<node_field_deref>(
        object.location.start,
        field_range.end,
        std::make_unique<node_expr>(std::move(object)),
        field_name.name,
        ILLEGAL_TYPE
    );

    if (capy_lexer->ahead_is_sym(token_symbol::sym_period))
    {
        node = parse_field_deref(std::move(node));
    }

    return node;
}

node_expr parser::parse_primary()
{
    if (capy_lexer->ahead_is<token_identifier>())
    {
        auto id = capy_lexer->expect<token_identifier>();
        auto id_range = id.location;

        if (capy_lexer->ahead_is_sym(token_symbol::sym_paren_open))
        {
            return parse_function_call(id_range, id.name);
        }
        // record initialisation and 'if' expressions can look very similar but we
        // can clearly distinguish them on a semantic level. When the identifier
        // before the opening '{' is a type, then this must be a record initialisation
        else if (current_scope->lookup_type(id.name).has_value() && capy_lexer->ahead_is_sym(token_symbol::sym_curly_open))
        {
            return parse_record_initialisation(id_range, id.name, false);
        }
        else if (capy_lexer->ahead_is_sym(token_symbol::sym_period))
        {
            auto var = current_scope->lookup(id.name);
            if (!var.has_value())
            {
                append_error_at(id_range.start, "Undefined variable: '" + id.name + "'");
                var = error_symbol;
            }

            node_expr object = make_located<node_var_reference>(
                id_range.start,
                id_range.end,
                id.name,
                var.value(),
                assign_context::rhs
            );

            return parse_field_deref(std::move(object));
        }
        else
        {
            auto var = current_scope->lookup(id.name);
            if (!var.has_value())
            {
                if ((id.name == "true") || (id.name == "false"))
                {
                    return make_located<node_bool_literal>(
                        id_range.start,
                        id_range.end,
                        bool_from_string(id.name)
                    );
                }

                append_error_at(id_range.start, "Undefined variable: '" + id.name + "'");
                var = error_symbol;
            }

            return make_located<node_var_reference>(
                id_range.start,
                id_range.end,
                id.name,
                var.value(),
                assign_context::rhs
            );
        }
    }
    else if (capy_lexer->ahead_is_sym(token_symbol::sym_kw_allocate))
    {
        capy_lexer->expect_symbol(token_symbol::sym_kw_allocate);
        if (capy_lexer->ahead_is<token_identifier>())
        {
            auto id = capy_lexer->expect<token_identifier>();
            auto id_range = id.location;

            if (current_scope->lookup_type(id.name).has_value() && capy_lexer->ahead_is_sym(token_symbol::sym_curly_open))
            {
                return parse_record_initialisation(id_range, id.name, true);
            }
        }
        else
        {
            append_error("After `allocate`, a record initialisation is required");
        }
        std::vector<node_field_initialisation> fields;
        return make_located<node_record_initialisation>(
            capy_lexer->current_source_position(),
            capy_lexer->current_source_position(),
            ILLEGAL_TYPE,
            std::move(fields)
        );
    }
    else if (capy_lexer->ahead_is_sym(token_symbol::sym_star))
    {
        auto op_token = capy_lexer->expect<token_symbol>();

        auto pointer_expr = parse_primary();

        return make_located<node_pointer_deref>(
            op_token.location.start,
            pointer_expr.location.end,
            std::make_unique<node_expr>(std::move(pointer_expr)),
            parse_context.create_type_var(),
            // parse_context.intern_primitive(primitive_type::Void),
            assign_context::rhs
        );
    }
    else if (capy_lexer->ahead_is_sym(token_symbol::sym_minus))
    {
        auto op_token = capy_lexer->expect<token_symbol>();
        auto expr = parse_expression(PRECEDENCE_UNARY_MINUS);

        auto* number = std::get_if<node_number>(&expr.value);
        if (number != nullptr)
        {
            number->number = -number->number;
            return expr;
        }
        else
        {
            return make_located<node_unary_expression>(
                op_token.location.start,
                expr.location.end,
                std::make_unique<node_expr>(std::move(expr)),
                operator_type::op_minus,
                parse_context.create_type_var()
            );
        }
    }
    else if (capy_lexer->ahead_is<token_integer>())
    {
        return parse_number();
    }
    else if (capy_lexer->ahead_is<token_string_literal>())
    {
        return parse_string();
    }
    else if (capy_lexer->ahead_is<token_char_literal>())
    {
        auto literal = capy_lexer->expect<token_char_literal>();
        auto literal_location = literal.location;
        return make_located<node_char_literal>(
            literal_location.start,
            literal_location.end,
            literal.ch
        );
    }
    else
    {
        append_error("Expected a primary (function call, number, variable)");
        // just create one of the simplest primaries in case we couldn't parse one
        return make_located<node_number>(
            capy_lexer->current_source_position(),
            capy_lexer->current_source_position(),
            0,
            parse_context.intern_primitive(primitive_type::U32)
        );
    }
}

type_id parser::parse_type_reference()
{
    bool is_pointer = false;
    if (capy_lexer->ahead_is_sym(token_symbol::sym_star))
    {
        is_pointer = true;
        capy_lexer->next_token();
    }

    source_range token_location;
    token_location.start = capy_lexer->current_source_position();
    token_location.end = capy_lexer->current_source_position();
    token_identifier type_name = {token_location, ""};

    std::optional<type_id> type_spec =
        parse_context.intern_primitive(primitive_type::Void);
    if (capy_lexer->ahead_is<token_identifier>())
    {
        type_name = capy_lexer->expect<token_identifier>();

        type_spec = type_from_id(parse_context, type_name.name);
        if (!type_spec.has_value())
        {
            type_spec = current_scope->lookup_type(type_name.name);
            if (!type_spec.has_value())
            {
                append_error("Unknown type specification: " + type_name.name);
                type_spec = parse_context.intern_primitive(primitive_type::Void);
            }
        }
    }
    else
    {
        append_error("Expecting an identifier for the type specification");
    }

    if (is_pointer)
    {
        auto new_type = pointer_type{type_spec.value()};
        type_spec = parse_context.intern(new_type);
    }

    return type_spec.value();
}

node_expr parser::parse_number()
{
    auto lhs = capy_lexer->expect<token_integer>();

    std::optional<type_id> number_type;
    if (lhs.type_suffix == "")
    {
        number_type = parse_context.create_type_var();
        parse_context.constraints.emplace_back(numeric_constraint{number_type.value()});
    }
    else
    {
        number_type = type_from_id(parse_context, lhs.type_suffix);
    }
    if (!number_type.has_value())
    {
        append_error("Number has an illegal suffix");
        return make_located<node_number>(
            capy_lexer->current_source_position(),
            lhs.location.end,
            0,
            parse_context.intern_primitive(primitive_type::U32)
        );
    }

    return make_located<node_number>(
        lhs.location.start,
        lhs.location.end,
        lhs.number,
        number_type.value()
    );
}

node_expr parser::parse_string()
{
    auto literal = capy_lexer->expect<token_string_literal>();
    auto literal_location = literal.location;
    auto literal_index = collect_literal(literal.str);
    return make_located<node_string_literal>(
        literal_location.start,
        literal_location.end,
        literal_index,
        literal.str.size()
    );
}

void parser::parse_body(std::vector<std::unique_ptr<node_expr>>& body)
{
    while (!capy_lexer->ahead_is_sym(token_symbol::sym_curly_close) &&
           !capy_lexer->ahead_is<token_eof>())
    {
        auto expression = parse_expression();
        body.emplace_back(std::make_unique<node_expr>(std::move(expression)));

        if (capy_lexer->ahead_is_sym(token_symbol::sym_semicolon))
        {
            capy_lexer->expect_symbol(token_symbol::sym_semicolon);

            // Keep explicit statement-vs-expression information without abusing
            // a cast-to-void wrapper.
            auto previous_expression = std::move(body.back());
            body.pop_back();

            auto discard_wrapper = make_located<node_discard_expression>(
                previous_expression->location.start,
                previous_expression->location.end,
                std::move(previous_expression)
            );
            body.emplace_back(std::make_unique<node_expr>(std::move(discard_wrapper)));
        }
    }

    if (!capy_lexer->ahead_is_sym(token_symbol::sym_curly_close))
    {
        append_error("Expecting a closing brace '}' to terminate the body");
    }
}

size_t parser::collect_literal(const std::string& literal)
{
    size_t insert_index = parse_context.string_literals.size();
    parse_context.string_literals.emplace_back(
        context::string_literal_entry{0, literal}
    );

    return insert_index;
}
