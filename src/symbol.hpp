#pragma once
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

// identity of a specific concrete type as an index into an array
// of type_node entries.
using type_id = std::uint32_t;
static constexpr type_id ILLEGAL_TYPE = 0;

struct pointer_type {
    type_id to;
};

struct record_type {
    using field_type = std::pair<std::string, type_id>;
    std::vector<field_type> fields;
};

class context;
struct function_type {
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

struct type_kind_hash {
    size_t operator()(type_kind const& t) const {
        size_t seed = t.index();

        std::visit(
            [&](auto const& x) {
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

struct type_kind_eq {
    bool operator()(type_kind const& a, type_kind const& b) const {
        if (a.index() != b.index())
            return false;

        return std::visit(
            [](auto const& x, auto const& y) -> bool {
                using typ_x = std::decay_t<decltype(x)>;
                using typ_y = std::decay_t<decltype(y)>;

                if constexpr (std::is_same_v<typ_x, primitive_type> &&
                    std::is_same_v<typ_y, primitive_type>)
                {
                    return x == y;
                }
                else if constexpr (std::is_same_v<typ_x, pointer_type> &&
                    std::is_same_v<typ_y, pointer_type>)
                {
                    return x.to == y.to;
                }
                else if constexpr (std::is_same_v<typ_x, record_type> &&
                    std::is_same_v<typ_y, record_type>)
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
                else if constexpr (std::is_same_v<typ_x, function_type> &&
                    std::is_same_v<typ_y, function_type>)
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
            a, b
        );
    }
};

struct type_var {
    std::optional<type_id> parent;
};

// type used to represent the shape (or later also potential shape) of
// types in one parsing pass
using type_node = std::variant<type_kind, type_var>;

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

    // Try to get the resolved type of a type_var by following through all the
    // parent references with a value. If a type variable cannot be resolved or
    // if it is not a type_var then type_idx will be returned.
    type_id resolved_type(type_id type_idx);

    std::optional<type_id> record_field_type(type_id record_type_idx, const std::string& field_name);
    std::optional<type_id> function_return_type(type_id function_type_idx);

    std::string repr(type_id type_idx) const;

    // indexing through type_id
    std::vector<type_node> types;
    std::unordered_map<type_kind, type_id, type_kind_hash, type_kind_eq> interned;
};

enum class symbol_kind {
    global_var,
    local_var,
    argument,
    function
};

struct function_signature
{
    std::vector<std::string> parameters;
    type_id function_type;
};

struct symbol {
    std::string name;
    type_id symbol_type;
    function_signature signature;
    symbol_kind kind;
    bool mutab;
    bool is_assigned;
};

struct scope {
    scope* parent;
    std::map<std::string, symbol> symbol_table;
    std::map<std::string, type_id> type_table;

    scope* get_global_scope() const;

    std::optional<std::reference_wrapper<symbol>> lookup(const std::string& name);
    std::optional<type_id> lookup_type(const std::string& name);
    std::optional<std::reference_wrapper<symbol>> lookup_function(const std::string& name);
};
