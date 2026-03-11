#pragma once
#include "parser.hpp"

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
    virtual void process(source_range location, node_expression& n) = 0;
    virtual void process(source_range location, node_if_expression& n) = 0;
    virtual void process(source_range location, node_while_expression& n) = 0;
    virtual void process(source_range location, node_let_expression& n) = 0;
    virtual void process(source_range location, node_global_definition& n) = 0;
    virtual void process(source_range location, node_function_definition& n) = 0;
    virtual void process(source_range location, node_import_definition& n) = 0;
    virtual void process(source_range location, node_module& n) = 0;

    assign_context current_context;
};
