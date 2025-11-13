#pragma once
#include <istream>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>
#include "lexer.hpp"

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

enum class symbol_kind {
    global_var,
    local_var,
    argument,
};

struct symbol {
    std::string name;
    type_kind symbol_type;
    symbol_kind kind;
    bool mutab;
    bool is_assigned;
    uint32_t index_addr;
};

struct func_symbol {
    std::string name;
    function_signature signature;
};

struct scope {
    scope* parent;
    std::map<std::string, symbol> symbol_table;
    std::map<std::string, type_kind> type_table;
    std::map<std::string, func_symbol> function_table;

    scope* get_global_scope() const;

    std::optional<std::reference_wrapper<symbol>> lookup(const std::string& name);
    std::optional<type_kind> lookup_type(const std::string& name);
    std::optional<func_symbol> lookup_function(const std::string& name);
};

template <typename T>
struct located
{
    T value;
    source_range location;
};

struct node_number;
struct node_char_literal;
struct node_bool_const;
struct node_string_literal;
struct node_var_reference;
struct node_pointer_deref;
struct node_let_expression;
struct node_if_expression;
struct node_while_expression;
struct node_type_spec;
struct node_record_definition;
struct node_record_initialisation;
struct node_field_deref;
struct node_function_head;
struct node_import_definition;
struct node_function_call;
struct node_function_definition;
struct node_expression;
struct node_module;

using ast_node_raw = std::variant<node_number, node_char_literal, node_bool_const, node_string_literal, node_var_reference, node_pointer_deref, node_let_expression, node_if_expression, node_while_expression, node_type_spec, node_record_definition, node_record_initialisation, node_field_deref, node_function_head, node_import_definition, node_function_call, node_function_definition, node_expression, node_module>;
using ast_node = located<ast_node_raw>;

void dump_ast(const ast_node& root, size_t indent=0);

enum class assign_context {
    lhs,
    rhs
};

struct node_number
{
    long long number;
    type_kind assigned_type;
};

struct node_char_literal
{
    uint32_t ch;
};

struct node_bool_const
{
    bool value;

    static bool from_string(const std::string& str) {
        return (str == "true");
    }
};

struct node_string_literal
{
    // index into the string literals table
    size_t table_index;
    uint32_t size;
};

struct node_var_reference
{
    std::string name;
    std::reference_wrapper<symbol> symbol_ref;
    assign_context context;
};

struct node_pointer_deref
{
    std::unique_ptr<ast_node> pointer_expression;
    type_kind assigned_type;
    assign_context context;
};

struct node_let_expression
{
    std::string name;
    type_kind assigned_type;
    std::reference_wrapper<symbol> symbol_ref;
    std::unique_ptr<ast_node> init_expression;
};

struct node_if_expression
{
    std::unique_ptr<ast_node> condition;
    std::vector<std::unique_ptr<ast_node>> then_code;
    std::vector<std::unique_ptr<ast_node>> else_code;
    type_kind assigned_type;
    // TODO: should be introduce a new scope for the if?
};

struct node_while_expression
{
    std::unique_ptr<ast_node> condition;
    std::vector<std::unique_ptr<ast_node>> while_code;
};

struct node_type_spec
{
    type_kind type_spec;
};

struct node_record_definition
{
    std::string name;
    std::vector<t_t::record::field_spec> fields;
};

struct node_field_deref
{
    std::unique_ptr<ast_node> object;
    std::string fieldname;
    type_kind object_type;

    // give a textual representation of the object for this dereferencing AST node
    // without including the field name
    std::string repr_obj() const;
};

struct field_initialisation
{
    source_position location;
    std::string field_name;
    std::unique_ptr<ast_node> init_expression;
};

struct node_record_initialisation
{
    type_kind type_spec;
    std::vector<field_initialisation> initialisations;
};

struct node_function_head
{
    std::string name;
    function_signature signature;
};

struct node_import_definition
{
    std::string ns_name;
    std::unique_ptr<ast_node> function_head;

    std::optional<std::string> alias;
};

struct node_function_call
{
    std::string function_name;
    func_symbol symbol_ref;
    std::vector<std::unique_ptr<ast_node>> parameter;
};

struct node_function_definition
{
    std::unique_ptr<ast_node> function_head;
    std::vector<std::unique_ptr<ast_node>> code;
    std::unique_ptr<scope> function_scope;
};

struct node_expression
{
    std::unique_ptr<ast_node> left, right;
    source_range op_range;
    operator_type operation;
    type_kind assigned_type;
};

struct node_module
{
    std::vector<std::unique_ptr<ast_node>> imports;
    std::vector<std::unique_ptr<ast_node>> functions;
    std::vector<std::unique_ptr<ast_node>> typedefs;

    struct string_literal_entry {
        uint32_t start_address;
        std::string literal;
    };
    std::vector<string_literal_entry> string_literals;

    std::unique_ptr<scope> module_scope;
};

struct parse_error
{
    source_position error_location;
    std::string error_message;
};

class parser
{
public:
    explicit parser(lexer &l);

    std::vector<parse_error> errors;
    ast_node parse();

private:
    lexer &capy_lexer;

    void append_error(const std::string &error_message);
    void append_error_at(source_position location, const std::string &error_message);

    ast_node parse_module();
    void parse_parameters(std::vector<param_spec>& parameters);
    void parse_function_signature(function_signature& signature);
    ast_node parse_import_definition();
    ast_node parse_function_definition();
    ast_node parse_function_head();
    ast_node parse_expression(int min_precedence = 0);
    ast_node parse_function_call(source_range name_range, const std::string function_name);
    ast_node parse_if_expression();
    ast_node parse_while_expression();
    ast_node parse_let_expression();
    ast_node parse_record_definition();
    ast_node parse_record_initialisation(source_range name_range, const std::string& record_name);
    ast_node parse_field_deref(type_kind base_type, ast_node object);
    ast_node parse_primary();
    ast_node parse_type_reference();
    ast_node parse_number();

    void parse_body(std::vector<std::unique_ptr<ast_node>>& body);

    size_t collect_literal(const std::string& literal);

    scope* current_scope;
    node_module* current_module;
};