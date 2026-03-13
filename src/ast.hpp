#pragma once
#include "lexer.hpp"
#include "symbol.hpp"
#include <memory>
#include <optional>
#include <variant>
#include <vector>

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

struct capy_attribute
{
    std::string name;
};

using attribute_bag = std::vector<capy_attribute>;

struct node_number;
struct node_char_literal;
struct node_bool_const;
struct node_string_literal;
struct node_var_reference;
struct node_pointer_deref;
struct node_let_expression;
struct node_if_expression;
struct node_while_expression;
struct node_record_definition;
struct node_record_initialisation;
struct node_field_deref;
struct node_function_head;
struct node_import_definition;
struct node_global_definition;
struct node_function_call;
struct node_function_definition;
struct node_cast_expression;
struct node_discard_expression;
struct node_return_expression;
struct node_break_statement;
struct node_negation;
struct node_expression;
struct node_module;

using ast_node_raw = std::variant<node_number, node_char_literal, node_bool_const, node_string_literal, node_var_reference, node_pointer_deref, node_let_expression, node_if_expression, node_while_expression, node_record_definition, node_record_initialisation, node_field_deref, node_function_call, node_cast_expression, node_discard_expression, node_return_expression, node_break_statement, node_negation, node_expression>;
using ast_node = located<ast_node_raw>;

void dump_module(const context& ctx, const node_module& module, size_t indent = 0);

enum class assign_context
{
    lhs,
    rhs
};

struct node_number
{
    long long number;
    type_id assigned_type;
};

struct node_char_literal
{
    uint32_t ch;
};

struct node_bool_const
{
    bool value;

    static bool from_string(const std::string& str)
    {
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
    symbol_id symbol_ref;
    assign_context context;
};

struct node_pointer_deref
{
    std::unique_ptr<ast_node> pointer_expression;
    type_id assigned_type;
    assign_context context;
};

struct node_let_expression
{
    std::string name;
    symbol_id symbol_ref;
    std::unique_ptr<ast_node> init_expression;
};

struct node_if_expression
{
    std::unique_ptr<ast_node> condition;
    std::vector<std::unique_ptr<ast_node>> then_code;
    std::vector<std::unique_ptr<ast_node>> else_code;
    type_id assigned_type;
    // TODO: should be introduce a new scope for the if?
};

struct node_while_expression
{
    std::unique_ptr<ast_node> condition;
    std::vector<std::unique_ptr<ast_node>> while_code;
};

struct node_record_definition
{
    std::string name;
    std::vector<record_type::field_type> fields;
};

struct node_field_deref
{
    std::unique_ptr<ast_node> object;
    std::string fieldname;
    type_id object_type;

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
    type_id type_spec;
    std::vector<field_initialisation> initialisations;
};

struct node_function_head : public located_node
{
    std::string name;
    function_signature signature;
};

struct node_import_definition : public located_node
{
    std::string ns_name;
    std::unique_ptr<node_function_head> function_head;

    std::optional<std::string> alias;
};

struct node_global_definition : public located_node
{
    std::string name;
    type_id assigned_type;
    symbol_id symbol_ref;
    int32_t init_value;
    // std::unique_ptr<ast_node> init_expression;
};

struct node_function_call
{
    std::string function_name;
    symbol_id symbol_ref;
    std::vector<std::unique_ptr<ast_node>> parameter;
};

struct node_function_definition : public located_node
{
    attribute_bag attributes;
    bool has_attribute(const std::string& attr_name) const;

    std::unique_ptr<node_function_head> function_head;
    std::vector<std::unique_ptr<ast_node>> code;
    std::unique_ptr<scope> function_scope;
};

struct node_cast_expression
{
    std::unique_ptr<ast_node> expression;
    type_id cast_type;
    source_range op_range;
};

struct node_discard_expression
{
    std::unique_ptr<ast_node> expression;
};

struct node_return_expression
{
    std::unique_ptr<ast_node> expression;
    // used to distinguish return expressions that come from explicit uses of
    // keyword 'return' or if it was implicitly created from the last expression
    // in a function
    bool is_explicit;
};

struct node_break_statement
{
};

struct node_negation
{
    std::unique_ptr<ast_node> expr;
    type_id assigned_type;
};

struct node_expression
{
    std::unique_ptr<ast_node> left, right;
    source_range op_range;
    operator_type operation;
    type_id assigned_type;
};

struct node_module : public located_node
{
    std::vector<std::unique_ptr<node_import_definition>> imports;
    std::vector<std::unique_ptr<node_global_definition>> globals;
    std::vector<std::unique_ptr<node_function_definition>> functions;
    std::vector<std::unique_ptr<ast_node>> typedefs;

    struct string_literal_entry
    {
        uint32_t start_address;
        std::string literal;
    };
    std::vector<string_literal_entry> string_literals;

    std::unique_ptr<scope> module_scope;
};

class ast_visitor
{
public:
    ast_visitor();
    void visit(ast_node& root);

protected:
    void visit_nodes(node_module& module);
    void visit_nodes(node_function_definition& func_def);
    void visit_nodes(node_if_expression& i_expr);
    void visit_nodes(node_while_expression& w_expr);
    void visit_nodes(node_let_expression& l_expr);
    void visit_nodes(node_negation& expr);
    void visit_nodes(node_expression& expr);
    void visit_nodes(node_cast_expression& expr);
    void visit_nodes(node_discard_expression& expr);
    void visit_nodes(node_return_expression& expr);
    void visit_nodes(node_function_call& func_call);
    void visit_nodes(node_record_initialisation& r_init);
    void visit_nodes(node_pointer_deref& ptr_deref);

    virtual void process(source_range location, node_number& n) = 0;
    virtual void process(source_range location, node_char_literal& n) = 0;
    virtual void process(source_range location, node_bool_const& n) = 0;
    virtual void process(source_range location, node_string_literal& n) = 0;
    virtual void process(source_range location, node_var_reference& n) = 0;
    virtual void process(source_range location, node_pointer_deref& n) = 0;
    virtual void process(source_range location, node_record_definition& n) = 0;
    virtual void process(source_range location, node_record_initialisation& n) = 0;
    virtual void process(source_range location, node_field_deref& n) = 0;
    virtual void process(source_range location, node_function_call& n) = 0;
    virtual void process(source_range location, node_cast_expression& n) = 0;
    virtual void process(source_range location, node_discard_expression& n) = 0;
    virtual void process(source_range location, node_return_expression& n) = 0;
    virtual void process(source_range location, node_break_statement& n) = 0;
    virtual void process(source_range location, node_negation& n) = 0;
    virtual void process(source_range location, node_expression& n) = 0;
    virtual void process(source_range location, node_if_expression& n) = 0;
    virtual void process(source_range location, node_while_expression& n) = 0;
    virtual void process(source_range location, node_let_expression& n) = 0;
    virtual void process(source_range location, node_global_definition& n) = 0;
    virtual void process(source_range location, node_function_definition& n) = 0;
    virtual void process(source_range location, node_import_definition& n) = 0;
    virtual void process(source_range location, node_module& n) = 0;

    assign_context current_context;
    size_t while_nesting_level;
};
