#pragma once
#include "ast_visitor.hpp"
#include "diagnostics.hpp"
#include "parser.hpp"

class type_inference : public ast_visitor, private diagnostic_emitter
{
public:
    explicit type_inference(context& ctx);
    void infer_types(node_module& module);

    const diagnostic_bag& diagnostics() const;

private:
    diagnostic_bag& diagnostics_sink() override;
    void unify();

    void process(source_range location, node_number& n) override;
    void process(source_range location, node_char_literal& n) override;
    void process(source_range location, node_bool_const& n) override;
    void process(source_range location, node_string_literal& n) override;
    void process(source_range location, node_var_reference& n) override;
    void process(source_range location, node_pointer_deref& n) override;
    void process(source_range location, node_record_definition& n) override;
    void process(source_range location, node_record_initialisation& n) override;
    void process(source_range location, node_field_deref& n) override;
    void process(source_range location, node_function_call& n) override;
    void process(source_range location, node_cast_expression& n) override;
    void process(source_range location, node_discard_expression& n) override;
    void process(source_range location, node_return_expression& n) override;
    void process(source_range location, node_break_statement& n) override;
    void process(source_range location, node_if_expression& n) override;
    void process(source_range location, node_expression& n) override;
    void process(source_range location, node_while_expression& n) override;
    void process(source_range location, node_let_expression& n) override;
    void process(source_range location, node_import_definition& n) override;
    void process(source_range location, node_global_definition& n) override;
    void process(source_range location, node_function_definition& n) override;
    void process(source_range location, node_module& n) override;

    context& parse_context;
    diagnostic_bag diagnostics_;
};
