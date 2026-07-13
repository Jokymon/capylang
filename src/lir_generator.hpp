#pragma once

#include "lir.hpp"
#include "ast.hpp"

class lir_generator
{
public:
    explicit lir_generator(context& ctx);

    lir::module generate(const node_module& module_def);

private:
    lir::expr_list generate_exprs(const node_number& node);
    lir::expr_list generate_exprs(const node_char_literal& node);
    lir::expr_list generate_exprs(const node_bool_literal& node);
    lir::expr_list generate_exprs(const node_string_literal& node);
    lir::expr_list generate_exprs(const node_var_reference& node);
    lir::expr_list generate_exprs(const node_pointer_deref& node);
    lir::expr_list generate_exprs(const node_if_expression& node);
    lir::expr_list generate_exprs(const node_record_definition& node);
    lir::expr_list generate_exprs(const node_record_initialisation& node);
    lir::expr_list generate_exprs(const node_field_deref& node);
    lir::expr_list generate_exprs(const node_function_call& node);
    lir::expr_list generate_exprs(const node_cast_expression& node);
    lir::expr_list generate_exprs(const node_unary_expression& node);
    lir::expr_list generate_exprs(const node_binary_expression& node);

    // The dispatching functions have a different name than the specialized
    // functions so that we can use "else"-dispatching for not-defined
    // specialisations
    lir::expr_list generate_to_exprs(const node_expr& expression);
    lir::statement_list generate_to_stmts(const node_expr& expression);

    lir::statement_list generate_stmts(const node_return_expression& node);
    lir::statement_list generate_stmts(const node_break_statement& node);
    lir::statement_list generate_stmts(const node_discard_expression& node);
    lir::statement_list generate_stmts(const node_let_expression& node);
    lir::statement_list generate_stmts(const node_while_expression& node);

    std::unique_ptr<lir::import_definition> generate(const node_import_definition& imp_def);
    std::unique_ptr<lir::global_definition> generate(const node_global_definition& global_def);
    std::unique_ptr<lir::function_definition> generate(const node_function_definition& func_def);

private:
    // For binary expressions representing assignments, we have to take a little
    // detour. The generate_exprs() function can only return an expr_list, but
    // in case of assignments, we actual need to return a store statement. To
    // make this work, the generate_exprs() function for binary_expressions
    // doesn't return anything in case of a store, but instead adds statements
    // to the assignments list. These are inserted by the generate_to_stmts
    // function after visiting all of its children.
    lir::statement_list assignments;
    context& parse_context;
};
