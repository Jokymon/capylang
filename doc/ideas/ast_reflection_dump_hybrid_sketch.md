# AST reflection dump sketch: hybrid metadata approach

This is a second sketch that keeps the main idea from
[`doc/ast_reflection_dump_sketch.md`](/C:/src/capylang/doc/ast_reflection_dump_sketch.md)
but changes how node-specific metadata is authored.

The goal here is:

- use reflection to enumerate members automatically
- use a small set of dump roles for the members that need special handling
- avoid large external `ast_dump_meta<T>::overrides()` blocks where possible
- keep "do not dump this field" information close to the struct definition

## 1. Main idea

There are really two different kinds of metadata:

1. Fields that need special dump behavior:
   - child node
   - child node list
   - type reference
   - symbol reference
   - location
2. Fields that should not be dumped at all:
   - `function_scope`
   - `module_scope`
   - potentially other caches or derived state later

Those two kinds should be modeled differently.

- special dump behavior can live in a small role table
- skipped fields can be declared locally in the node type

That gives a hybrid design with low ceremony.

## 2. First naming cleanup

Your AST currently has two concepts:

- `node_xxx` structs, which are all AST node shapes
- `ast_node = located<std::variant<...>>`, which is only the expression-capable
  subset stored inside the variant

That naming makes generic dumping harder to talk about. For the dumper I would
use this terminology:

- `expr_node` or `variant_node`
  for the current `ast_node`
- `ast shape`
  for any `node_xxx` struct
- `child`
  for a nested node field

You do not need to rename the code immediately, but the dumper internals should
avoid using names like `node_child` versus `ast_child` if both are AST nodes in
practice.

A cleaner role set would be:

```cpp
enum class dump_role
{
    scalar,
    child,
    child_list,
    type_ref,
    symbol_ref,
    location,
    skip
};
```

`child` means:

- `ast_node`
- `std::unique_ptr<ast_node>`
- `node_function_head`
- `std::unique_ptr<node_function_head>`
- any other nested `node_xxx`

`child_list` means vectors of those.

## 3. The simplest useful solution

The most practical design is:

- reflection enumerates all fields
- a generic classifier handles the easy cases by type
- each node type may optionally expose:
  - a node name
  - a skip list
  - a role override list

That means most nodes need no metadata at all.

## 4. Put skip information in the struct

This is the part that maps best to your idea.

Example:

```cpp
struct node_function_definition : public located_node
{
    static constexpr std::string_view dump_name = "function_definition";
    static constexpr auto dump_skip_fields = std::to_array<std::string_view>({
        "function_scope",
    });

    attribute_bag attributes;
    std::unique_ptr<node_function_head> function_head;
    std::vector<std::unique_ptr<ast_node>> code;
    std::unique_ptr<scope> function_scope;
};
```

And:

```cpp
struct node_module : public located_node
{
    static constexpr std::string_view dump_name = "module";
    static constexpr auto dump_skip_fields = std::to_array<std::string_view>({
        "module_scope",
    });

    std::vector<std::unique_ptr<node_import_definition>> imports;
    std::vector<std::unique_ptr<node_global_definition>> globals;
    std::vector<std::unique_ptr<node_function_definition>> functions;
    std::vector<std::unique_ptr<ast_node>> typedefs;
    std::vector<string_literal_entry> string_literals;
    std::unique_ptr<scope> module_scope;
};
```

This is simple, explicit, and compiler-friendly.

## 5. Can `NO_DUMP(...)` be made to work?

Yes, but with limits.

### Option A: declaration macro only

This is possible:

```cpp
#define NO_DUMP_FIELD(Type, Name) Type Name
```

Usage:

```cpp
NO_DUMP_FIELD(std::unique_ptr<scope>, function_scope);
```

But this alone does not help reflection. It only changes the spelling of the
declaration. The dumper still needs some metadata elsewhere saying that
`function_scope` is skipped.

So this form is cosmetic only unless paired with some registration mechanism.

### Option B: local skip-list macro

This is the useful macro form:

```cpp
#define AST_DUMP_SKIPS(...) \
    static constexpr auto dump_skip_fields = std::to_array<std::string_view>({ __VA_ARGS__ })
```

Usage:

```cpp
struct node_function_definition : public located_node
{
    static constexpr std::string_view dump_name = "function_definition";
    AST_DUMP_SKIPS("function_scope");

    attribute_bag attributes;
    std::unique_ptr<node_function_head> function_head;
    std::vector<std::unique_ptr<ast_node>> code;
    std::unique_ptr<scope> function_scope;
};
```

This is probably the best fit for your goal:

- the skip metadata is inside the type
- the syntax cost is low
- no fragile per-member declaration wrappers are needed
- the generic dumper can query it easily

### Option C: wrapped field type

You could also invent a wrapper:

```cpp
template <typename T>
struct no_dump
{
    T value;
};
```

and write:

```cpp
no_dump<std::unique_ptr<scope>> function_scope;
```

This makes skipping easy by type trait, but it changes the field type
everywhere. That is too invasive for a simple dumping concern, so I would not do
it here.

### Option D: impossible macro fantasy

A macro like:

```cpp
NO_DUMP(std::unique_ptr<scope>) function_scope;
```

cannot by itself create a hidden registration entry tied to `function_scope`
without extra declarations. The preprocessor sees tokens, not reflection
entities. It cannot "attach" durable semantic metadata to the later declarator
name in a way ordinary C++ reflection will recover for free.

So this form is not a good foundation.

## 6. Recommended hybrid API

I would use three optional static members per node type:

```cpp
template <typename T>
struct dump_type_info
{
    static constexpr std::string_view name = "node";
    static constexpr auto skip_fields = std::to_array<std::string_view>({});
    static constexpr auto role_overrides = std::tuple{};
};
```

But instead of specializing that externally, define the information directly in
the node when practical:

```cpp
struct node_var_reference
{
    static constexpr std::string_view dump_name = "var_reference";

    std::string name;
    symbol_id symbol_ref;
    assign_context context;
};
```

```cpp
struct node_function_definition : public located_node
{
    static constexpr std::string_view dump_name = "function_definition";
    AST_DUMP_SKIPS("function_scope");

    attribute_bag attributes;
    std::unique_ptr<node_function_head> function_head;
    std::vector<std::unique_ptr<ast_node>> code;
    std::unique_ptr<scope> function_scope;
};
```

Then use external metadata only for the exceptional role overrides.

## 7. Role overrides should be terse

Instead of a verbose `overrides()` function, use member-pointer-based entries:

```cpp
template <auto MemberPtr>
struct dump_role_override
{
    static constexpr auto member = MemberPtr;
    static constexpr dump_role role = dump_role::scalar;
};

template <auto MemberPtr, dump_role Role>
struct dump_role_entry
{
    static constexpr auto member = MemberPtr;
    static constexpr dump_role role = Role;
};
```

Per type:

```cpp
struct node_var_reference
{
    static constexpr std::string_view dump_name = "var_reference";
    static constexpr auto dump_roles = std::tuple{
        dump_role_entry<&node_var_reference::symbol_ref, dump_role::symbol_ref>{},
    };

    std::string name;
    symbol_id symbol_ref;
    assign_context context;
};
```

```cpp
struct node_function_definition : public located_node
{
    static constexpr std::string_view dump_name = "function_definition";
    AST_DUMP_SKIPS("function_scope");
    static constexpr auto dump_roles = std::tuple{
        dump_role_entry<&node_function_definition::function_head, dump_role::child>{},
        dump_role_entry<&node_function_definition::code, dump_role::child_list>{},
    };

    attribute_bag attributes;
    std::unique_ptr<node_function_head> function_head;
    std::vector<std::unique_ptr<ast_node>> code;
    std::unique_ptr<scope> function_scope;
};
```

That is much shorter than external `ast_dump_meta<T>` specializations and keeps
the metadata near the fields it describes.

## 8. Generic classification should do most of the work

The dumper should not need explicit overrides for every child field.

For example:

```cpp
template <typename T>
struct is_child_like : std::false_type {};

template <>
struct is_child_like<ast_node> : std::true_type {};

template <typename T>
struct is_child_like<std::unique_ptr<T>> : is_child_like<T> {};
```

and maybe:

```cpp
template <>
struct is_child_like<node_function_head> : std::true_type {};

template <>
struct is_child_like<node_import_definition> : std::true_type {};

template <>
struct is_child_like<node_global_definition> : std::true_type {};

template <>
struct is_child_like<node_function_definition> : std::true_type {};
```

Then:

```cpp
template <typename T>
constexpr dump_role default_dump_role_for()
{
    if constexpr (std::same_as<T, type_id>)
    {
        return dump_role::type_ref;
    }
    else if constexpr (std::same_as<T, symbol_id>)
    {
        return dump_role::symbol_ref;
    }
    else if constexpr (std::same_as<T, source_range> || std::same_as<T, source_position>)
    {
        return dump_role::location;
    }
    else if constexpr (is_vector_v<T> && is_child_like<typename T::value_type>::value)
    {
        return dump_role::child_list;
    }
    else if constexpr (is_child_like<T>::value)
    {
        return dump_role::child;
    }
    else
    {
        return dump_role::scalar;
    }
}
```

With that in place, only a small number of node types need `dump_roles` at all.

## 9. How skip lookup would work

Reflection gives you the member name. The node type provides a `dump_skip_fields`
array. Then:

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

So the object dumper becomes:

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
            constexpr dump_role role = effective_role<T, member, decltype(field)>();
            dump_member<role>(ctx, field, indent + 2, member_name);
        }
    }
}
```

## 10. Suggested convenience macros

These are about the maximum amount of macro magic I would use:

```cpp
#define AST_DUMP_NAME(Name) \
    static constexpr std::string_view dump_name = Name

#define AST_DUMP_SKIPS(...) \
    static constexpr auto dump_skip_fields = std::to_array<std::string_view>({ __VA_ARGS__ })

#define AST_DUMP_ROLES(...) \
    static constexpr auto dump_roles = std::tuple{ __VA_ARGS__ }
```

Usage:

```cpp
struct node_module : public located_node
{
    AST_DUMP_NAME("module");
    AST_DUMP_SKIPS("module_scope");
    AST_DUMP_ROLES(
        dump_role_entry<&node_module::imports, dump_role::child_list>{},
        dump_role_entry<&node_module::globals, dump_role::child_list>{},
        dump_role_entry<&node_module::functions, dump_role::child_list>{},
        dump_role_entry<&node_module::typedefs, dump_role::child_list>{}
    );

    std::vector<std::unique_ptr<node_import_definition>> imports;
    std::vector<std::unique_ptr<node_global_definition>> globals;
    std::vector<std::unique_ptr<node_function_definition>> functions;
    std::vector<std::unique_ptr<ast_node>> typedefs;
    std::vector<string_literal_entry> string_literals;
    std::unique_ptr<scope> module_scope;
};
```

That is terse enough to be maintainable and still explicit.

## 11. Recommendation

I would not pursue a macro that wraps the field declaration itself.

I would pursue this:

1. Reflection enumerates members.
2. Type-based classification handles most fields automatically.
3. `AST_DUMP_SKIPS(...)` keeps skip metadata local to the node definition.
4. `AST_DUMP_ROLES(...)` is used only when automatic classification is not good
   enough.

That gives you:

- local node metadata
- much less boilerplate than full `ast_dump_meta<T>` specialization blocks
- no dependence on attribute introspection support
- no invasive field wrapper types

## 12. Minimal example

```cpp
struct node_pointer_deref
{
    AST_DUMP_NAME("pointer_deref");

    std::unique_ptr<ast_node> pointer_expression;
    type_id assigned_type;
    assign_context context;
};

struct node_function_definition : public located_node
{
    AST_DUMP_NAME("function_definition");
    AST_DUMP_SKIPS("function_scope");

    attribute_bag attributes;
    std::unique_ptr<node_function_head> function_head;
    std::vector<std::unique_ptr<ast_node>> code;
    std::unique_ptr<scope> function_scope;
};
```

In that model:

- `pointer_expression` is auto-detected as `child`
- `assigned_type` is auto-detected as `type_ref`
- `function_scope` is skipped by local metadata
- no external specialization is needed unless you want to override something
