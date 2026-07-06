#pragma once

#include "lir.hpp"
#include "ast.hpp"

class lir_generator
{
public:
    explicit lir_generator(context& ctx);

    lir::module generate(const node_module& module_def);

private:
    lir::expr_list generate(const node_number& node);
    lir::expr_list generate(const node_char_literal& node);
    lir::expr_list generate(const node_bool_literal& node);
    lir::expr_list generate(const node_string_literal& node);
    lir::expr_list generate(const node_var_reference& node);
    lir::expr_list generate(const node_pointer_deref& node);
    lir::expr_list generate(const node_let_expression& node);
    lir::expr_list generate(const node_if_expression& node);
    lir::expr_list generate(const node_while_expression& node);
    lir::expr_list generate(const node_record_definition& node);
    lir::expr_list generate(const node_record_initialisation& node);
    lir::expr_list generate(const node_field_deref& node);
    lir::expr_list generate(const node_function_call& node);
    lir::expr_list generate(const node_cast_expression& node);
    lir::expr_list generate(const node_discard_expression& node);
    lir::expr_list generate(const node_return_expression& node);
    lir::expr_list generate(const node_unary_expression& node);
    lir::expr_list generate(const node_binary_expression& node);
    lir::expr_list generate(const node_break_statement& node);
    lir::expr_list generate(const node_expr& expression);

    std::unique_ptr<lir::import_definition> generate(const node_import_definition& imp_def);
    std::unique_ptr<lir::global_definition> generate(const node_global_definition& global_def);
    std::unique_ptr<lir::function_definition> generate(const node_function_definition& func_def);

private:
    context& parse_context;
};
