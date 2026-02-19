#include "type_inference.hpp"
#include "semantics.hpp"
#include <ranges>

type_inference::type_inference(context& ctx)
: parse_context(ctx)
{

}

void type_inference::infer_types(node_module& module)
{
    for (const auto &import_def : module.imports)
    {
        visit(*import_def);
    }

    for (const auto &global_def : module.globals)
    {
        visit(*global_def);
    }

    for (const auto &type_def : module.typedefs)
    {
        visit(*type_def);
    }

    for (const auto &func_def : module.functions)
    {
        visit(*func_def);
    }

    unify();
}

void type_inference::unify()
{
    size_t new_unresolved = parse_context.constraints.size();
    size_t previous_unresolved;

    do {
        new_unresolved = previous_unresolved;
        for (const auto& constraint: parse_context.constraints)
        {
            std::visit([&](const auto& constr){
                    using T = std::decay_t<decltype(constr)>;

                    if constexpr (std::is_same_v<T, equal_constraint>)
                    {
                        if (parse_context.resolve(constr.a, constr.b))
                        {
                            previous_unresolved--;
                        }
                    }
                },
                constraint);
        }
    } while (new_unresolved!=previous_unresolved);
}

void type_inference::visit(ast_node &root)
{
    std::visit([&](auto &n) -> void {
        process(root.location, n);
    }, root.value);
}

void type_inference::process(source_range location, node_number &n)
{
    (void)location;
    (void)n;
}

void type_inference::process(source_range location, node_char_literal &n)
{
    (void)location;
    (void)n;
}

void type_inference::process(source_range location, node_bool_const &n)
{
    (void)location;
    (void)n;
}

void type_inference::process(source_range location, node_string_literal &n)
{
    (void)location;
    (void)n;
}

void type_inference::process(source_range location, node_var_reference &n)
{
    (void)location;
    (void)n;
}

void type_inference::process(source_range location, node_pointer_deref &n)
{
    (void)location;
    visit(*n.pointer_expression);
}

void type_inference::process(source_range location, node_record_definition &n)
{
    (void)location;
    (void)n;
}

void type_inference::process(source_range location, node_record_initialisation &n)
{
    (void)location;
    for (auto &field_init : n.initialisations)
    {
        visit(*field_init.init_expression);
    }
}

void type_inference::process(source_range location, node_field_deref &n)
{
    (void)location;
    visit(*n.object);
}

void type_inference::process(source_range location, node_function_call &n)
{
    (void)location;
    const auto& called_symbol = parse_context.symbol_at(n.symbol_ref);
    if (called_symbol.kind != symbol_kind::function)
    {
        return;
    }

    auto declared_type = parse_context.types[called_symbol.signature.function_type];
    auto declared_function_type = std::get<function_type>(std::get<type_kind>(declared_type));

    for (auto [actual_value, declared_param]: std::views::zip(n.parameter, declared_function_type.parameter_types))
    {
        visit(*actual_value);

        parse_context.constraints.emplace_back(equal_constraint{
            assigned_node_type(*actual_value, parse_context),
            declared_param
        });
    }
}

void type_inference::process(source_range location, node_if_expression &n)
{
    (void)location;
    visit(*n.condition);

    for (auto &expression : n.then_code)
    {
        visit(*expression);
    }

    for (auto &expression : n.else_code)
    {
        visit(*expression);
    }
}

void type_inference::process(source_range location, node_while_expression &n)
{
    (void)location;
    visit(*n.condition);

    for (auto &expression : n.while_code)
    {
        visit(*expression);
    }
}

void type_inference::process(source_range location, node_let_expression &n)
{
    (void)location;
    if (!n.init_expression)
    {
        return;
    }
    visit(*n.init_expression);

    parse_context.constraints.emplace_back(equal_constraint{
        parse_context.symbol_at(n.symbol_ref).symbol_type,
        assigned_node_type(*n.init_expression, parse_context)
    });
}

void type_inference::process(source_range location, node_import_definition &n)
{
    (void)location;
    (void)n;
}

void type_inference::process(source_range location, node_global &n)
{
    (void)location;
    (void)n;
}

void type_inference::process(source_range location, node_function_definition &n)
{
    (void)location;
    for (auto &expression : n.code)
    {
        visit(*expression);
    }
}

void type_inference::process(source_range location, node_cast_expression &n)
{
    (void)location;
    visit(*n.expression);
}

void type_inference::process(source_range location, node_expression &n)
{
    (void)location;
    visit(*n.left);
    visit(*n.right);
}
