#pragma once
#include <istream>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>
#include "lexer.hpp"
#include "symbol.hpp"

template <typename T>
struct located
{
    T value;
    source_range location;
};

struct located_node
{
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
struct node_global;
struct node_function_call;
struct node_function_definition;
struct node_expression;
struct node_module;

using ast_node_raw = std::variant<node_number, node_char_literal, node_bool_const, node_string_literal, node_var_reference, node_pointer_deref, node_let_expression, node_if_expression, node_while_expression, node_type_spec, node_record_definition, node_record_initialisation, node_field_deref, node_import_definition, node_global, node_function_call, node_function_definition, node_expression>;
using ast_node = located<ast_node_raw>;

void dump_module(const node_module& module, size_t indent=0);

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

struct node_function_head : public located_node
{
    std::string name;
    function_signature signature;
};

struct node_import_definition
{
    std::string ns_name;
    std::unique_ptr<node_function_head> function_head;

    std::optional<std::string> alias;
};

struct node_global
{
    std::string name;
    type_kind assigned_type;
    std::reference_wrapper<symbol> symbol_ref;
    int32_t init_value;
    //std::unique_ptr<ast_node> init_expression;
};

struct node_function_call
{
    std::string function_name;
    symbol symbol_ref;
    std::vector<std::unique_ptr<ast_node>> parameter;
};

struct node_function_definition
{
    std::unique_ptr<node_function_head> function_head;
    std::vector<std::unique_ptr<ast_node>> code;
    std::unique_ptr<scope> function_scope;
    bool exported;
};

struct node_expression
{
    std::unique_ptr<ast_node> left, right;
    source_range op_range;
    operator_type operation;
    type_kind assigned_type;
};

struct node_module : public located_node
{
    std::vector<std::unique_ptr<ast_node>> imports;
    std::vector<std::unique_ptr<ast_node>> globals;
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
    node_module parse();

private:
    lexer &capy_lexer;

    void append_error(const std::string &error_message);
    void append_error_at(source_position location, const std::string &error_message);

    node_module parse_module();
    void parse_attribute();
    void parse_parameters(std::vector<param_spec>& parameters);
    void parse_function_signature(function_signature& signature);
    ast_node parse_import_definition();
    ast_node parse_global();
    ast_node parse_function_definition();
    node_function_head parse_function_head();
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