# AST reflection-based dump sketch

This is a sketch for replacing the hand-written `dump_node(...)` functions in
[`src/ast.cpp`](/C:/src/capylang/src/ast.cpp) with one generic YAML-like dumper.

The design goal is:

- keep `dump_ast(ctx, ast_node, indent)` as the public entry point
- derive field names automatically
- recurse automatically into `ast_node`, `unique_ptr<ast_node>`, and vectors
- still allow AST-specific formatting for `type_id`, `symbol_id`, locations, and
  fields that should be skipped such as scopes

## Important constraint

True standard C++26 compile-time reflection is still compiler-dependent in
practice. The sketch below therefore uses the proposed reflection style
(`std::meta`, `nonstatic_data_members_of`, `identifier_of`, `extract`) as the
conceptual model, plus a tiny metadata layer for the AST-specific parts.

If your compiler does not yet support that reflection syntax, the same overall
shape can be implemented with explicit member metadata tables instead.

## 1. Add a tiny AST dump metadata layer

The dumper needs more than raw reflection. Some fields need context-aware
formatting, some are child nodes, and some should be hidden.

```cpp
enum class dump_role
{
    scalar,
    ast_child,
    ast_child_list,
    node_child,
    node_child_list,
    type_ref,
    symbol_ref,
    location,
    skip
};

template <typename Owner, typename Member>
struct dump_field
{
    std::string_view name;
    Member Owner::*ptr;
    dump_role role = dump_role::scalar;
};

template <typename T>
struct ast_dump_meta
{
    static constexpr std::string_view node_name = "node";
    static constexpr auto overrides()
    {
        return std::tuple{};
    }
};
```

The metadata is only for overrides. Most fields are discovered automatically by
reflection.

## 2. Give each node an optional node name and per-field overrides

For many structs, only a few fields need annotation.

```cpp
struct node_var_reference
{
    static constexpr std::string_view dump_name = "var_reference";

    std::string name;
    symbol_id symbol_ref;
    assign_context context;
};

template <>
struct ast_dump_meta<node_var_reference>
{
    static constexpr std::string_view node_name = "var_reference";

    static constexpr auto overrides()
    {
        using T = node_var_reference;
        return std::to_array({
            dump_field<T, symbol_id>{"symbol_ref", &T::symbol_ref, dump_role::symbol_ref},
        });
    }
};
```

For fields that should not be printed:

```cpp
template <>
struct ast_dump_meta<node_function_definition>
{
    static constexpr std::string_view node_name = "function_definition";

    static constexpr auto overrides()
    {
        using T = node_function_definition;
        return std::to_array({
            dump_field<T, std::unique_ptr<scope>>{"function_scope", &T::function_scope, dump_role::skip},
            dump_field<T, std::unique_ptr<node_function_head>>{"function_head", &T::function_head, dump_role::node_child},
            dump_field<T, std::vector<std::unique_ptr<ast_node>>>{"code", &T::code, dump_role::ast_child_list},
        });
    }
};

template <>
struct ast_dump_meta<node_module>
{
    static constexpr std::string_view node_name = "module";

    static constexpr auto overrides()
    {
        using T = node_module;
        return std::to_array({
            dump_field<T, std::vector<std::unique_ptr<node_import_definition>>>{"imports", &T::imports, dump_role::node_child_list},
            dump_field<T, std::vector<std::unique_ptr<node_global_definition>>>{"globals", &T::globals, dump_role::node_child_list},
            dump_field<T, std::vector<std::unique_ptr<node_function_definition>>>{"functions", &T::functions, dump_role::node_child_list},
            dump_field<T, std::vector<std::unique_ptr<ast_node>>>{"typedefs", &T::typedefs, dump_role::ast_child_list},
            dump_field<T, std::unique_ptr<scope>>{"module_scope", &T::module_scope, dump_role::skip},
        });
    }
};
```

The same pattern works for `type_id`, `source_range`, and `symbol_id`.

## 3. Generic dumper entry points

This keeps the current user-facing API.

```cpp
void dump_ast(const context& ctx, const ast_node& root, size_t indent = 0);
void dump_module(const context& ctx, const node_module& module, size_t indent = 0);
```

Implementation:

```cpp
void dump_ast(const context& ctx, const ast_node& root, size_t indent)
{
    dump_value(ctx, root, indent, "ast");
}

void dump_module(const context& ctx, const node_module& module, size_t indent)
{
    dump_value(ctx, module, indent, ast_dump_meta<node_module>::node_name);
}
```

## 4. Generic dumping primitives

The dumper is built around a small set of overloads:

```cpp
void write_indent(size_t indent)
{
    std::cout << std::string(indent, ' ');
}

template <typename T>
void dump_scalar(const context& ctx, const T& value)
{
    if constexpr (std::same_as<T, bool>)
    {
        std::cout << (value ? "true" : "false");
    }
    else if constexpr (std::same_as<T, std::string>)
    {
        std::cout << '"' << value << '"';
    }
    else if constexpr (std::same_as<T, type_id>)
    {
        std::cout << '"' << ctx.repr(value) << '"';
    }
    else if constexpr (std::same_as<T, symbol_id>)
    {
        if (value == ILLEGAL_SYMBOL)
        {
            std::cout << "null";
        }
        else
        {
            const auto& sym = ctx.symbol_at(value);
            std::cout << "{ id: " << value
                      << ", name: \"" << sym.name
                      << "\", type: \"" << ctx.repr(sym.symbol_type) << "\" }";
        }
    }
    else if constexpr (std::same_as<T, assign_context>)
    {
        std::cout << (value == assign_context::lhs ? "\"lhs\"" : "\"rhs\"");
    }
    else if constexpr (std::same_as<T, operator_type>)
    {
        std::cout << '"' << repr_op(value) << '"';
    }
    else
    {
        std::cout << value;
    }
}
```

Then the recursive overload set:

```cpp
template <typename T>
void dump_value(const context& ctx, const std::unique_ptr<T>& ptr, size_t indent, std::string_view name)
{
    write_indent(indent);
    std::cout << name << ":";
    if (!ptr)
    {
        std::cout << " null\n";
        return;
    }
    std::cout << "\n";
    dump_value(ctx, *ptr, indent + 2, ast_dump_meta<T>::node_name);
}

void dump_value(const context& ctx, const ast_node& node, size_t indent, std::string_view name)
{
    write_indent(indent);
    std::cout << name << ":\n";
    std::visit(
        [&](const auto& concrete)
        {
            dump_value(ctx, concrete, indent + 2, ast_dump_meta<std::decay_t<decltype(concrete)>>::node_name);
        },
        node.value
    );
}

template <typename T>
void dump_value(const context& ctx, const std::vector<T>& items, size_t indent, std::string_view name)
{
    write_indent(indent);
    std::cout << name << ":\n";
    for (const auto& item : items)
    {
        write_indent(indent + 2);
        std::cout << "-\n";
        dump_value(ctx, item, indent + 4, "item");
    }
}
```

## 5. Reflection-based object dumper

This is the part that replaces most of the hand-written dumpers.

```cpp
template <typename T>
consteval auto reflected_members()
{
    return std::meta::nonstatic_data_members_of(^T);
}

template <typename T>
constexpr dump_role override_role_for(std::string_view name)
{
    for (const auto& field : ast_dump_meta<T>::overrides())
    {
        if (field.name == name)
        {
            return field.role;
        }
    }
    return dump_role::scalar;
}

template <typename T>
void dump_value(const context& ctx, const T& object, size_t indent, std::string_view name)
{
    write_indent(indent);
    std::cout << name << ":\n";

    template for (constexpr auto member : reflected_members<T>())
    {
        constexpr auto member_name = std::meta::identifier_of(member);
        constexpr auto role = override_role_for<T>(member_name);

        if constexpr (role != dump_role::skip)
        {
            const auto& field = [: member :](object);
            dump_member<role>(ctx, field, indent + 2, member_name);
        }
    }
}
```

`dump_member` is a small dispatcher:

```cpp
template <dump_role Role, typename Field>
void dump_member(const context& ctx, const Field& field, size_t indent, std::string_view name)
{
    if constexpr (Role == dump_role::ast_child || Role == dump_role::node_child)
    {
        dump_value(ctx, field, indent, name);
    }
    else if constexpr (Role == dump_role::ast_child_list || Role == dump_role::node_child_list)
    {
        dump_value(ctx, field, indent, name);
    }
    else
    {
        write_indent(indent);
        std::cout << name << ": ";
        dump_scalar(ctx, field);
        std::cout << "\n";
    }
}
```

## 6. What this would look like for the current AST

With minimal metadata, these nodes become fully generic:

- `node_number`
- `node_char_literal`
- `node_bool_const`
- `node_string_literal`
- `node_let_expression`
- `node_if_expression`
- `node_while_expression`
- `node_function_call`
- `node_cast_expression`
- `node_return_expression`
- `node_negation`
- `node_expression`

Nodes that still need a few overrides:

- `node_var_reference`
  because `symbol_ref` should be resolved through `context`
- `node_pointer_deref`
  because `pointer_expression` is an AST child
- `node_record_definition`
  because `record_type::field_type` should be printed nicely
- `node_function_definition`
  because `function_scope` should be skipped
- `node_module`
  because `imports`, `globals`, `functions`, and `typedefs` are child lists and
  `module_scope` should be skipped

## 7. Example output

For a small let-expression, the output could become:

```yaml
ast:
  let_expression:
    name: "x"
    symbol_ref: { id: 17, name: "x", type: "s32" }
    init_expression:
      number:
        number: 42
        assigned_type: "s32"
```

And for an if-expression:

```yaml
ast:
  if_expression:
    assigned_type: "s32"
    condition:
      var_reference:
        name: "flag"
        symbol_ref: { id: 8, name: "flag", type: "bool" }
        context: "rhs"
    then_code:
      -
        number:
          number: 1
          assigned_type: "s32"
    else_code:
      -
        number:
          number: 0
          assigned_type: "s32"
```

## 8. Practical recommendation for this codebase

A reasonable migration path here would be:

1. Keep `dump_ast(ctx, ast_node, indent)` and `dump_module(ctx, node_module, indent)`.
2. Introduce a generic dumper in a new file, for example
   `src/ast_dump_reflect.hpp`.
3. Start with explicit `ast_dump_meta<T>` overrides only for:
   - `node_module`
   - `node_function_definition`
   - `node_function_head`
   - `node_import_definition`
   - `node_global_definition`
   - `node_var_reference`
   - `node_pointer_deref`
   - `node_record_definition`
   - `node_record_initialisation`
   - `node_field_deref`
4. Let reflection handle the rest automatically.
5. Once the generic path is stable, delete the hand-written `dump_node(...)`
   overload set from [`src/ast.cpp`](/C:/src/capylang/src/ast.cpp).

## 9. If reflection support is not usable yet

The same design still works if `reflected_members<T>()` is replaced by explicit
member tables:

```cpp
template <>
struct ast_dump_meta<node_let_expression>
{
    static constexpr std::string_view node_name = "let_expression";

    static constexpr auto members()
    {
        using T = node_let_expression;
        return std::to_array({
            dump_field<T, std::string>{"name", &T::name, dump_role::scalar},
            dump_field<T, symbol_id>{"symbol_ref", &T::symbol_ref, dump_role::symbol_ref},
            dump_field<T, std::unique_ptr<ast_node>>{"init_expression", &T::init_expression, dump_role::ast_child},
        });
    }
};
```

That fallback still removes the printing logic duplication. The only remaining
manual part is describing members, not writing YAML output code per node type.
