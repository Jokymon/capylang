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
    struct s32;
    struct u32;
    struct pointer;
};
using type_kind = std::variant<t_t::unassigned, t_t::void_type, t_t::s32, t_t::u32, t_t::pointer>;

namespace t_t
{
    struct unassigned{
        bool operator==(const unassigned& other) const { return true; }
    };

    struct void_type{
        bool operator==(const void_type& other) const { return true; }
    };
    struct s32{
        bool operator==(const s32& other) const { return true; }
    };
    struct u32{
        bool operator==(const u32& other) const { return true; }
    };

    struct pointer{
        pointer(const type_kind& base_type);
        pointer(const pointer& other);
        pointer& operator=(const pointer& other);

        bool operator==(const pointer& other) const;

        std::unique_ptr<type_kind> base_type;
    };

    template<typename T, typename V>
    static bool is_of(V&& value) {
        return std::holds_alternative<T>(value);
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
    uint32_t index_addr;
};

struct func_symbol {
    std::string name;
    function_signature signature;
};

struct scope {
    scope* parent;
    std::map<std::string, symbol> symbol_table;
    std::map<std::string, func_symbol> function_table;

    std::optional<symbol> lookup(const std::string& name);
    std::optional<func_symbol> lookup_function(const std::string& name);
};

struct node_number;
struct node_var_reference;
struct node_pointer_deref;
struct node_let_expression;
struct node_type_spec;
struct node_function_head;
struct node_import_definition;
struct node_function_call;
struct node_function_definition;
struct node_expression;
struct node_module;
struct node_parse_error;

using ast_node_raw = std::variant<node_number, node_var_reference, node_pointer_deref, node_let_expression, node_type_spec, node_function_head, node_import_definition, node_function_call, node_function_definition, node_expression, node_module, node_parse_error>;
using ast_node = located<ast_node_raw>;

struct node_number
{
    long long number;
    type_kind assigned_type;
};

struct node_var_reference
{
    std::string name;
    symbol symbol_ref;
};

struct node_pointer_deref
{
    std::unique_ptr<ast_node> pointer_expression;
    type_kind assigned_type;
};

struct node_let_expression
{
    std::string name;
    type_kind assigned_type;
    symbol symbol_ref;
    std::unique_ptr<ast_node> init_expression;
};

struct node_type_spec
{
    type_kind type_spec;
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

    std::unique_ptr<scope> module_scope;
};

struct node_parse_error
{
    // TODO: the location is now duplicated from located<ast>
    source_position error_location;
    std::string error_message;
    // A node representing a failed parsing result
};

bool is_error(const ast_node &node);

class parser
{
public:
    explicit parser(lexer &l);

    ast_node parse();

private:
    lexer &capy_lexer;

    ast_node create_error(const std::string &error_message);

    ast_node parse_module();
    std::optional<ast_node> parse_parameters(std::vector<param_spec>& parameters);
    std::optional<ast_node> parse_function_signature(function_signature& signature);
    ast_node parse_import_definition();
    ast_node parse_function_definition();
    ast_node parse_function_head();
    ast_node parse_expression(int min_precedence = 0);
    ast_node parse_function_call(const std::string function_name);
    ast_node parse_let_expression();
    ast_node parse_primary();
    ast_node parse_type_reference();
    ast_node parse_number();

    scope* current_scope;
};