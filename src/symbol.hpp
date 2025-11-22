#pragma once
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace t_t
{
    struct unassigned;
    struct void_type;
    struct char_type;
    struct boolean;
    struct u8;
    struct s32;
    struct u32;
    struct string;
    struct pointer;
    struct record;
};
using type_kind = std::variant<t_t::unassigned, t_t::void_type, t_t::char_type, t_t::boolean, t_t::s32, t_t::u8, t_t::u32, t_t::string, t_t::pointer, t_t::record>;

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
    struct s32{
        bool operator==(const s32& other) const { return true; }
    };
    struct u8{
        bool operator==(const u8& other) const { return true; }
    };
    struct u32{
        bool operator==(const u32& other) const { return true; }
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
    function,
    type_spec
};

struct param_spec
{
    std::string name;
    type_kind type_spec;
};

struct function_signature
{
    std::vector<param_spec> parameters;
    type_kind return_type;

    bool equals_call_signature(function_signature& other);
    std::string repr();
};

struct symbol {
    std::string name;
    type_kind symbol_type;
    function_signature signature;
    symbol_kind kind;
    bool mutab;
    bool is_assigned;
    uint32_t index_addr;
};

struct scope {
    scope* parent;
    std::map<std::string, symbol> symbol_table;

    scope* get_global_scope() const;

    std::optional<std::reference_wrapper<symbol>> lookup(const std::string& name);
    std::optional<type_kind> lookup_type(const std::string& name);
    std::optional<symbol> lookup_function(const std::string& name);
};
