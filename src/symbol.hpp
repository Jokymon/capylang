#pragma once
#include "locations.hpp"
#include <compare>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

enum class primitive_type
{
    Void,
    Char,
    Boolean,
    U8,
    U16,
    U32,
    S8,
    S16,
    S32,
    String
};

template <typename Tag>
struct strong_id
{
    using underlying_type = std::uint32_t;

    underlying_type value = 0;

    constexpr strong_id() = default;
    constexpr explicit strong_id(underlying_type v)
    : value(v)
    {
    }

    constexpr auto operator<=>(const strong_id&) const = default;
};

template <typename Tag>
constexpr size_t to_index(strong_id<Tag> id)
{
    return static_cast<size_t>(id.value);
}

template <typename Tag>
constexpr std::uint32_t to_underlying(strong_id<Tag> id)
{
    return id.value;
}

// identity of a specific concrete type as an index into an array
// of type_node entries.
struct type_id_tag
{
};
using type_id = strong_id<type_id_tag>;
static constexpr type_id ILLEGAL_TYPE{0};

// identity of a symbol in the global symbol arena in context.
struct symbol_id_tag
{
};
using symbol_id = strong_id<symbol_id_tag>;
static constexpr symbol_id ILLEGAL_SYMBOL{0};

struct pointer_type
{
    type_id to;
};

struct record_type
{
    using field_type = std::pair<std::string, type_id>;
    std::vector<field_type> fields;
};

class context;
struct function_type
{
    type_id return_type;
    std::vector<type_id> parameter_types;

    bool is_call_signature_eq(const function_type& other);

    std::string repr_call_sig(const context& ctx) const;
};

// description of the shape of a type
using type_kind = std::variant<
    primitive_type,
    pointer_type,
    record_type,
    function_type
>;

inline void hash_combine(size_t& seed, size_t value)
{
    seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
}

template <typename Tag>
inline void hash_combine(size_t& seed, strong_id<Tag> value)
{
    hash_combine(seed, to_index(value));
}

struct type_kind_hash
{
    size_t operator()(type_kind const& t) const
    {
        size_t seed = t.index();

        std::visit(
            [&](auto const& x)
            {
                using typ_x = std::decay_t<decltype(x)>;

                if constexpr (std::is_same_v<typ_x, primitive_type>)
                {
                    hash_combine(seed, static_cast<size_t>(x));
                }
                else if constexpr (std::is_same_v<typ_x, pointer_type>)
                {
                    hash_combine(seed, x.to);
                }
                else if constexpr (std::is_same_v<typ_x, record_type>)
                {
                    for (auto const& [name, tid] : x.fields)
                    {
                        hash_combine(seed, std::hash<std::string>{}(name));
                        hash_combine(seed, tid);
                    }
                }
                else if constexpr (std::is_same_v<typ_x, function_type>)
                {
                    hash_combine(seed, x.return_type);
                    for (auto const& param_tid : x.parameter_types)
                    {
                        hash_combine(seed, param_tid);
                    }
                }
            },
            t
        );

        return seed;
    }
};

struct type_kind_eq
{
    bool operator()(type_kind const& a, type_kind const& b) const
    {
        if (a.index() != b.index())
            return false;

        return std::visit(
            [](auto const& x, auto const& y) -> bool
            {
                using typ_x = std::decay_t<decltype(x)>;
                using typ_y = std::decay_t<decltype(y)>;

                if constexpr (std::is_same_v<typ_x, primitive_type> && std::is_same_v<typ_y, primitive_type>)
                {
                    return x == y;
                }
                else if constexpr (std::is_same_v<typ_x, pointer_type> && std::is_same_v<typ_y, pointer_type>)
                {
                    return x.to == y.to;
                }
                else if constexpr (std::is_same_v<typ_x, record_type> && std::is_same_v<typ_y, record_type>)
                {
                    if (x.fields.size() != y.fields.size())
                    {
                        return false;
                    }
                    for (size_t i = 0; i < x.fields.size(); ++i)
                    {
                        if (x.fields[i].first != y.fields[i].first)
                            return false;
                        if (x.fields[i].second != y.fields[i].second)
                            return false;
                    }
                    return true;
                }
                else if constexpr (std::is_same_v<typ_x, function_type> && std::is_same_v<typ_y, function_type>)
                {
                    if (x.return_type != y.return_type)
                    {
                        return false;
                    }
                    if (x.parameter_types.size() != y.parameter_types.size())
                    {
                        return false;
                    }
                    for (size_t i = 0; i < x.parameter_types.size(); ++i)
                    {
                        if (x.parameter_types[i] != y.parameter_types[i])
                            return false;
                    }
                    return true;
                }
                else
                {
                    return false;
                }
            },
            a,
            b
        );
    }
};

struct type_var
{
    std::optional<type_id> parent;
};

struct equal_constraint
{
    type_id a, b;
};

struct numeric_constraint
{
    type_id n;
};

using type_constraint = std::variant<
    equal_constraint,
    numeric_constraint
>;

// type used to represent the shape (or later also potential shape) of
// types in one parsing pass
using type_node = std::variant<type_kind, type_var>;

template <class T>
const T* get_type_from_node(const type_node& node)
{
    const auto* kind = std::get_if<type_kind>(&node);
    if (!kind)
        return nullptr;

    return std::get_if<T>(kind);
}

enum class symbol_kind
{
    error,
    global_var,
    local_var,
    argument,
    function
};

struct function_parameter
{
    source_range location;
    std::string name;
};

struct function_signature
{
    std::vector<function_parameter> parameters;
    type_id function_type;
};

struct symbol
{
    std::string name;
    source_range location;
    type_id symbol_type;
    function_signature signature;
    symbol_kind kind;
    bool mutab;
    bool is_assigned;
    bool is_intrinsic;
};

struct context
{
    context();

    type_id intern_primitive(primitive_type p_type);
    type_id intern(const type_kind& type);
    type_id create_type_var();

    bool is_primitive_type(type_id type_idx, primitive_type p_type);
    bool is_record_type(type_id type_idx);
    bool is_pointer_type(type_id type_idx);
    bool is_type_var(type_id type_idx);

    bool is_resolved(type_id type_idx) const;
    // Try to get the resolved type of a type_var by following through all the
    // parent references with a value. If a type variable cannot be resolved or
    // if it is not a type_var then type_idx will be returned.
    type_id resolved_type(type_id type_idx) const;
    bool resolve(type_id idx1, type_id idx2);

    // Syntactically dereferencings of records and records behind pointers look
    // the same and end up in the same AST nodes, we need a way to get to the
    // actual record definition easily for both cases. The function `record_behind()`
    // does exactly that and either gives us the record type id for the given record
    // type or it "dereferences" the pointer to a record and gives that record type.
    std::optional<type_id> record_behind(type_id record_or_pointer_type);
    // find the base type for the given type index; find_base_type() expects a
    // resolved type rather than a type variable. For pointer types the function
    // removes all the pointer elements until it reaches a non-pointer pointer.
    std::optional<type_id> find_base_type(type_id pointer_type_idx);
    std::optional<type_id> record_field_type(type_id record_type_idx, const std::string& field_name);
    std::optional<type_id> function_return_type(type_id function_type_idx);
    std::optional<primitive_type> primitive_type_of(type_id type_idx) const;

    std::string repr(type_id type_idx) const;

    symbol_id create_symbol(symbol sym);
    symbol& symbol_at(symbol_id idx);
    const symbol& symbol_at(symbol_id idx) const;

    // indexing through type_id
    std::vector<type_node> types;
    std::unordered_map<type_kind, type_id, type_kind_hash, type_kind_eq> interned;

    std::vector<type_constraint> constraints;
    std::vector<symbol> symbols;

    struct string_literal_entry
    {
        uint32_t start_address;
        std::string literal;
    };
    std::vector<string_literal_entry> string_literals;
};

struct scope
{
    scope* parent;
    std::map<std::string, symbol_id> symbol_table;
    std::map<std::string, type_id> type_table;

    scope* get_global_scope() const;

    std::optional<symbol_id> lookup(const std::string& name);
    std::optional<type_id> lookup_type(const std::string& name);
};

namespace std
{
    template <typename Tag>
    struct hash<strong_id<Tag>>
    {
        size_t operator()(strong_id<Tag> id) const noexcept
        {
            return std::hash<std::uint32_t>{}(id.value);
        }
    };
} // namespace std
