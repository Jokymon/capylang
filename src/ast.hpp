#pragma once
#include "lexer.hpp"
#include "symbol.hpp"
#include <memory>
#include <optional>
#include <ostream>
#include <variant>
#include <vector>

template <typename T>
struct located
{
    T value;
    source_range location;
};

struct node_base
{
};

struct located_node : public node_base
{
    source_range location;
};

struct capy_attribute
{
    std::string name;
};

using attribute_bag = std::vector<capy_attribute>;

struct located_node_with_attributes : public located_node
{
    attribute_bag attributes;

    bool has_attribute(const std::string& attr_name) const;
};

struct node_number;
struct node_char_literal;
struct node_bool_literal;
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
struct node_unary_expression;
struct node_binary_expression;
struct node_module;

using node_expr_raw = std::variant<node_number, node_char_literal, node_bool_literal, node_string_literal, node_var_reference, node_pointer_deref, node_let_expression, node_if_expression, node_while_expression, node_record_definition, node_record_initialisation, node_field_deref, node_function_call, node_cast_expression, node_discard_expression, node_return_expression, node_break_statement, node_unary_expression, node_binary_expression>;
using node_expr = located<node_expr_raw>;

void dump_node(std::ostream& os, const context& ctx, const node_module& module, int indent = 0);
void dump_node(std::ostream& os, const context& ctx, const node_expr& node, int indent = 0);

enum class assign_context
{
    lhs,
    rhs
};

#define DEFINE_NODES
#include "dumpable.hpp"
#include "ast_nodes.hpp"
#undef DEFINE_NODES

class ast_visitor
{
public:
    ast_visitor();
    void visit(node_expr& root);

protected:
    void visit_nodes(node_module& module);
    void visit_nodes(node_function_definition& func_def);
    void visit_nodes(node_if_expression& i_expr);
    void visit_nodes(node_while_expression& w_expr);
    void visit_nodes(node_let_expression& l_expr);
    void visit_nodes(node_unary_expression& expr);
    void visit_nodes(node_binary_expression& expr);
    void visit_nodes(node_cast_expression& expr);
    void visit_nodes(node_discard_expression& expr);
    void visit_nodes(node_return_expression& expr);
    void visit_nodes(node_function_call& func_call);
    void visit_nodes(node_record_initialisation& r_init);
    void visit_nodes(node_pointer_deref& ptr_deref);

    virtual void process(source_range location, node_number& n) = 0;
    virtual void process(source_range location, node_char_literal& n) = 0;
    virtual void process(source_range location, node_bool_literal& n) = 0;
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
    virtual void process(source_range location, node_unary_expression& n) = 0;
    virtual void process(source_range location, node_binary_expression& n) = 0;
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
