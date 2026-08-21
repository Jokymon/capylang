#pragma once

#include "lir.hpp"
#include "ast.hpp"

class lir_generator
{
public:
    explicit lir_generator(context& ctx);

    lir::module generate(const ast::module& module_def);

private:
    lir::expr_list generate_exprs(const ast::number& node);
    lir::expr_list generate_exprs(const ast::char_literal& node);
    lir::expr_list generate_exprs(const ast::bool_literal& node);
    lir::expr_list generate_exprs(const ast::string_literal& node);
    lir::expr_list generate_exprs(const ast::var_reference& node);
    lir::expr_list generate_exprs(const ast::pointer_deref& node);
    lir::expr_list generate_exprs(const ast::if_expression& node);
    lir::expr_list generate_exprs(const ast::record_definition& node);
    lir::expr_list generate_exprs(const ast::record_initialisation& node);
    lir::expr_list generate_exprs(const ast::field_deref& node);
    lir::expr_list generate_exprs(const ast::function_call& node);
    lir::expr_list generate_exprs(const ast::cast_expression& node);
    lir::expr_list generate_exprs(const ast::unary_expression& node);
    lir::expr_list generate_exprs(const ast::binary_expression& node);

    // The dispatching functions have a different name than the specialized
    // functions so that we can use "else"-dispatching for not-defined
    // specialisations
    lir::expr_list generate_to_exprs(const ast::expr& expression);
    lir::statement_list generate_to_stmts(const ast::expr& expression);

    lir::statement_list generate_stmts(const ast::return_expression& node);
    lir::statement_list generate_stmts(const ast::break_statement& node);
    lir::statement_list generate_stmts(const ast::discard_expression& node);
    lir::statement_list generate_stmts(const ast::let_expression& node);
    lir::statement_list generate_stmts(const ast::while_expression& node);

    std::unique_ptr<lir::import_definition> generate(const ast::import_definition& imp_def);
    std::unique_ptr<lir::global_definition> generate(const ast::global_definition& global_def);
    std::unique_ptr<lir::function_definition> generate(const ast::function_definition& func_def);

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
