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
using type_kind2 = std::variant<
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
    size_t operator()(type_kind2 const& t) const {
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
    bool operator()(type_kind2 const& a, type_kind2 const& b) const {
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
using type_node = std::variant<type_kind2, type_var>;

struct context
{
    context();

    type_id intern_primitive(primitive_type p_type);
    type_id intern(const type_kind2& type);
    type_id create_type_var();

    bool is_primitive_type(type_id type_idx, primitive_type p_type);
    bool is_record_type(type_id type_idx);
    bool is_pointer_type(type_id type_idx);
    bool is_type_var(type_id type_idx);

    std::optional<type_id> record_field_type(type_id record_type_idx, const std::string& field_name);
    std::optional<type_id> function_return_type(type_id function_type_idx);

    std::string repr(type_id type_idx) const;

    // indexing through type_id
    std::vector<type_node> types;
    std::unordered_map<type_kind2, type_id, type_kind_hash, type_kind_eq> interned;
};

namespace t_t
{
    struct unassigned;
    struct void_type;
    struct char_type;
    struct boolean;
    struct u8;
    struct u16;
    struct u32;
    struct s8;
    struct s16;
    struct s32;
    struct string;
    struct pointer;
    struct record;
};
using type_kind = std::variant<t_t::unassigned, t_t::void_type, t_t::char_type, t_t::boolean, t_t::u8, t_t::u16, t_t::u32, t_t::s8, t_t::s16, t_t::s32, t_t::string, t_t::pointer, t_t::record>;

namespace t_t
{
    struct unassigned{
        bool operator==(const unassigned& other) const { return true; }
    };

    struct void_type{
        bool operator==(const void_type& other) const { return true; }
    };
    struct char_type{
        bool operator==(const char_type& other) const { return true; }
    };
    struct boolean{
        bool operator==(const boolean& other) const { return true; }
    };
    struct u8{
        bool operator==(const u8& other) const { return true; }
    };
    struct u16{
        bool operator==(const u16& other) const { return true; }
    };
    struct u32{
        bool operator==(const u32& other) const { return true; }
    };
    struct s8{
        bool operator==(const s8& other) const { return true; }
    };
    struct s16{
        bool operator==(const s16& other) const { return true; }
    };
    struct s32{
        bool operator==(const s32& other) const { return true; }
    };
    struct string{
        bool operator==(const string& other) const { return true; }
    };

    struct pointer{
        pointer(const type_kind& base_type);
        pointer(const pointer& other);
        pointer& operator=(const pointer& other);

        bool operator==(const pointer& other) const;

        std::unique_ptr<type_kind> base_type;
    };

    struct record{
        struct field_spec {
            field_spec(const std::string& name, std::unique_ptr<type_kind> type_spec);
            field_spec(const field_spec& other);
            field_spec& operator=(const field_spec& other);

            std::string name;
            std::unique_ptr<type_kind> type_spec;
        };

        record(const std::vector<field_spec>& fields);
        record(const record& other);
        record& operator=(const record& other);

        bool operator==(const record& other) const;

        std::optional<type_kind> field_type(const std::string& name);

        std::vector<field_spec> fields;
    };

    template<typename T, typename V>
    static bool is_of(V&& value) {
        return std::holds_alternative<T>(value);
    }

    static bool is_record_like(type_kind t) {
        return std::holds_alternative<t_t::record>(t) ||
                std::holds_alternative<t_t::string>(t);
    }
};

std::string repr_type(const type_kind& type_spec);

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
    std::map<std::string, type_id> type_table2;

    scope* get_global_scope() const;

    std::optional<std::reference_wrapper<symbol>> lookup(const std::string& name);
    std::optional<type_id> lookup_type2(const std::string& name);
    std::optional<std::reference_wrapper<symbol>> lookup_function(const std::string& name);
};
