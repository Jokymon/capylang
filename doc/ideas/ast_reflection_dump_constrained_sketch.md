# AST reflection dump sketch: constrained category-based dumper

This sketch assumes the current AST layout in
[`src/ast.hpp`](/C:/src/capylang/src/ast.hpp):

- every concrete AST node struct derives from `node_base`
- location-carrying top-level nodes derive from `located_node`
- expression nodes inside expressions are wrapped as
  `node_expr = located<std::variant<...>>`

The proposed dumping model is deliberately strict:

- reflection enumerates members
- `dump_member(...)` is only defined for a small set of supported categories
- unsupported member types fail at compile time
- hidden implementation fields are skipped explicitly via `AST_DUMP_SKIPS(...)`

This keeps the generic machinery small and makes unsupported cases obvious.

## 1. Core idea

There are three distinct kinds of things in the current AST:

1. Concrete node structs such as `node_pointer_deref`,
   `node_function_definition`, and `node_module`
2. Expression wrappers, represented by `node_expr`
3. Non-node payload types such as `type_id`, `symbol_id`, `source_range`,
   `function_signature`, `capy_attribute`, and `field_initialisation`

The dumper should treat those differently.

- concrete node structs are handled by one generic reflective object dumper
- `node_expr` gets its own variant-aware dumper
- non-node payload types must either have an explicit `dump_member` overload or
  be skipped

That is enough to remove the hand-written per-node `dump_*` functions without
trying to make the dumper fully generic for arbitrary C++ objects.

## 2. Supported categories

The effective categories are:

```cpp
enum class dump_category
{
    scalar,
    concrete_node,
    concrete_node_ptr,
    concrete_node_list,
    expr_node,
    expr_node_ptr,
    expr_node_list,
    type_ref,
    symbol_ref,
    location,
    skipped
};
```

In practice, the implementation does not need to expose this enum publicly. It
is enough that `dump_member(...)` has overloads for these categories.

## 3. Node identity comes from `node_base`

The refactoring makes the structural part simple.

```cpp
template <typename T>
concept concrete_ast_node =
    std::derived_from<std::remove_cvref_t<T>, node_base>;
```

That gives direct support for:

- `T` where `T : node_base`
- `std::unique_ptr<T>` where `T : node_base`
- `std::vector<std::unique_ptr<T>>` where `T : node_base`

Since `located_node` already derives from `node_base`, this also covers:

- `node_function_head`
- `node_import_definition`
- `node_global_definition`
- `node_function_definition`
- `node_module`

There is no need for extra role metadata just to say that these are child nodes.

## 4. `node_expr` stays a dedicated category

`node_expr` is not a regular node struct. It is:

```cpp
using node_expr = located<node_expr_raw>;
```

So it still deserves its own dumping path:

```cpp
void dump_member(const context& ctx, const node_expr& node, size_t indent, std::string_view name);
void dump_member(const context& ctx, const std::unique_ptr<node_expr>& node, size_t indent, std::string_view name);
void dump_member(const context& ctx, const std::vector<std::unique_ptr<node_expr>>& nodes, size_t indent, std::string_view name);
```

That keeps the distinction clear:

- `node_base` means a concrete AST node struct
- `node_expr` means the located expression-variant wrapper

## 5. Reflection only enumerates members

The reflective part should stay very small.

```cpp
template <typename T>
void dump_object(const context& ctx, const T& object, size_t indent, std::string_view name)
{
    write_indent(indent);
    std::cout << name << ":\n";

    template for (constexpr auto member : std::meta::nonstatic_data_members_of(^T))
    {
        constexpr auto member_name = std::meta::identifier_of(member);

        if constexpr (!is_skipped_field<T>(member_name))
        {
            const auto& field = [: member :](object);
            dump_member(ctx, field, indent + 2, member_name);
        }
    }
}
```

This function does not classify fields. It only:

- iterates over members
- checks the local skip list
- forwards to `dump_member(...)`

That separation is the main strength of this design.

## 6. `dump_member(...)` is the policy boundary

The primary template should fail intentionally:

```cpp
template <typename T>
void dump_member(const context&, const T&, size_t, std::string_view)
{
    static_assert(always_false_v<T>,
        "No AST dump support for this member type. "
        "Add a dump_member overload or list the field in AST_DUMP_SKIPS(...).");
}
```

Then add overloads only for the categories the AST actually needs.

### Scalar leaves

```cpp
void dump_member(const context&, const std::string& value, size_t indent, std::string_view name);
void dump_member(const context&, bool value, size_t indent, std::string_view name);
void dump_member(const context&, long long value, size_t indent, std::string_view name);
void dump_member(const context&, size_t value, size_t indent, std::string_view name);
void dump_member(const context&, uint32_t value, size_t indent, std::string_view name);
void dump_member(const context&, int32_t value, size_t indent, std::string_view name);
void dump_member(const context&, assign_context value, size_t indent, std::string_view name);
void dump_member(const context&, operator_type value, size_t indent, std::string_view name);
```

### Semantic IDs

```cpp
void dump_member(const context& ctx, type_id value, size_t indent, std::string_view name);
void dump_member(const context& ctx, symbol_id value, size_t indent, std::string_view name);
```

These are not dumped as raw integers. They are formatted semantically through
`context`.

### Locations

```cpp
void dump_member(const context&, const source_position& value, size_t indent, std::string_view name);
void dump_member(const context&, const source_range& value, size_t indent, std::string_view name);
```

### Concrete AST nodes

```cpp
template <concrete_ast_node T>
void dump_member(const context& ctx, const T& node, size_t indent, std::string_view name)
{
    dump_object(ctx, node, indent, name_of<T>());
}

template <concrete_ast_node T>
void dump_member(const context& ctx, const std::unique_ptr<T>& node, size_t indent, std::string_view name)
{
    write_indent(indent);
    std::cout << name << ":";
    if (!node)
    {
        std::cout << " null\n";
        return;
    }
    std::cout << "\n";
    dump_object(ctx, *node, indent + 2, name_of<T>());
}

template <concrete_ast_node T>
void dump_member(const context& ctx, const std::vector<std::unique_ptr<T>>& nodes, size_t indent, std::string_view name)
{
    write_indent(indent);
    std::cout << name << ":\n";
    for (const auto& node : nodes)
    {
        write_indent(indent + 2);
        std::cout << "-\n";
        dump_member(ctx, node, indent + 4, "item");
    }
}
```

### `node_expr`

```cpp
void dump_member(const context& ctx, const node_expr& node, size_t indent, std::string_view name)
{
    write_indent(indent);
    std::cout << name << ":\n";
    std::visit(
        [&](const auto& concrete)
        {
            dump_object(ctx, concrete, indent + 2, name_of<std::decay_t<decltype(concrete)>>());
        },
        node.value
    );
}
```

The pointer and vector forms follow the same pattern.

## 7. Skip metadata is local and explicit

`AST_DUMP_SKIPS(...)` is part of the design, not a workaround.

```cpp
#define AST_DUMP_NAME(Name) \
    static constexpr std::string_view dump_name = Name

#define AST_DUMP_SKIPS(...) \
    static constexpr auto dump_skip_fields = std::to_array<std::string_view>({ __VA_ARGS__ })
```

Usage:

```cpp
struct node_function_definition : public located_node
{
    AST_DUMP_NAME("function_definition");
    AST_DUMP_SKIPS("function_scope");

    attribute_bag attributes;
    std::unique_ptr<node_function_head> function_head;
    std::vector<std::unique_ptr<node_expr>> code;
    std::unique_ptr<scope> function_scope;
};
```

```cpp
struct node_module : public located_node
{
    AST_DUMP_NAME("module");
    AST_DUMP_SKIPS("module_scope");

    std::vector<std::unique_ptr<node_import_definition>> imports;
    std::vector<std::unique_ptr<node_global_definition>> globals;
    std::vector<std::unique_ptr<node_function_definition>> functions;
    std::vector<std::unique_ptr<node_expr>> typedefs;
    std::vector<string_literal_entry> string_literals;
    std::unique_ptr<scope> module_scope;
};
```

Skip lookup can be name-based:

```cpp
template <typename T>
constexpr bool is_skipped_field(std::string_view member_name)
{
    if constexpr (requires { T::dump_skip_fields; })
    {
        for (auto name : T::dump_skip_fields)
        {
            if (name == member_name)
            {
                return true;
            }
        }
    }
    return false;
}
```

That is enough for the current design.

## 8. Where this removes metadata entirely

With the current inheritance structure, all of these become automatic:

- `node_pointer_deref::pointer_expression`
- `node_let_expression::init_expression`
- `node_if_expression::condition`
- `node_if_expression::then_code`
- `node_if_expression::else_code`
- `node_while_expression::condition`
- `node_while_expression::while_code`
- `node_function_definition::function_head`
- `node_function_definition::code`
- `node_module::imports`
- `node_module::globals`
- `node_module::functions`
- `node_module::typedefs`

They no longer need role tables because the member types already encode the
category:

- `std::unique_ptr<node_expr>`
- `std::vector<std::unique_ptr<node_expr>>`
- `std::unique_ptr<T>` where `T : node_base`
- `std::vector<std::unique_ptr<T>>` where `T : node_base`

## 9. Types that still need dedicated dump support

A few payload types are not scalar and not node-like:

- `function_signature`
- `attribute_bag`
- `capy_attribute`
- `field_initialisation`
- `record_type::field_type`
- `node_module::string_literal_entry`

Those should be handled deliberately. Each of them can take one of three paths:

1. add a dedicated `dump_member(...)` overload
2. allow them to be dumped reflectively as normal helper structs
3. skip them explicitly

For example:

- `attribute_bag` probably deserves a small custom list formatter
- `field_initialisation` is a good candidate for reflective dumping
- `function_signature` may want a context-aware representation

## 10. Main strengths

This design has four strong properties.

### 1. The generic core stays small

Reflection only enumerates members. It does not carry policy logic.

### 2. The type system does most of the work

The current `node_base` / `located_node` / `node_expr` structure is already
strong enough to classify most members without additional metadata.

### 3. Unsupported cases fail loudly

The primary `dump_member(...)` template turns forgotten cases into compile-time
errors instead of silently bad output.

### 4. Skip metadata stays local

`AST_DUMP_SKIPS(...)` keeps "this field is implementation state" close to the
struct definition where it belongs.

## 11. Main limitations

The limitations are narrow and acceptable.

### 1. `node_expr` still needs dedicated support

That is inherent in the `located<std::variant<...>>` representation.

### 2. Skip lists are name-based

`AST_DUMP_SKIPS("function_scope")` depends on the reflected member name string.
That is acceptable here, but it is not type-safe.

### 3. Helper payload types still require choices

The non-node aggregate types listed above still need an explicit dumping policy.

### 4. Inherited members need verification

Since `located_node` carries `location`, the implementation should verify
exactly how the chosen reflection API exposes inherited data members and whether
that matches the intended output.

## 12. Minimal example

```cpp
struct node_pointer_deref : public node_base
{
    AST_DUMP_NAME("pointer_deref");

    std::unique_ptr<node_expr> pointer_expression;
    type_id assigned_type;
    assign_context context;
};

struct node_function_definition : public located_node
{
    AST_DUMP_NAME("function_definition");
    AST_DUMP_SKIPS("function_scope");

    attribute_bag attributes;
    std::unique_ptr<node_function_head> function_head;
    std::vector<std::unique_ptr<node_expr>> code;
    std::unique_ptr<scope> function_scope;
};
```

Under this model:

- `pointer_expression` works because it is `std::unique_ptr<node_expr>`
- `assigned_type` works because it is `type_id`
- `function_head` works because it is `std::unique_ptr<T>` with `T : node_base`
- `code` works because it is `std::vector<std::unique_ptr<node_expr>>`
- `function_scope` is skipped locally
- unsupported new member types fail at compile time until they get a real policy

## 13. Recommendation

For the current AST shape, this is the direction I would choose.

1. Keep `node_base`, `located_node`, and `node_expr` exactly as the structural
   categories the dumper understands.
2. Implement a constrained overload set for `dump_member(...)`.
3. Make the primary template a compile-time error.
4. Use `AST_DUMP_SKIPS(...)` as the standard local mechanism for hidden fields.
5. Add targeted overloads for helper payload types only when they are actually
   needed.

That should get you a generic AST dumper with very little per-node metadata and
a clear failure mode when the AST evolves.
