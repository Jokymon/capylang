#pragma once
#include "diagnostics.hpp"
#include "parser.hpp"
#include "symbol.hpp"
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

enum class anf_binary_op
{
    add,
    sub,
    mul,
    div,
    mod,
    eq,
    ne
};

struct anf_binding
{
    std::string name;
    symbol_id symbol_ref = ILLEGAL_SYMBOL;
    type_id assigned_type = ILLEGAL_TYPE;
};

struct anf_atom_var
{
    anf_binding binding;
};

struct anf_atom_number
{
    int64_t value;
    type_id assigned_type = ILLEGAL_TYPE;
};

struct anf_atom_bool
{
    bool value;
};

struct anf_atom_char
{
    uint32_t ch;
};

struct anf_atom_string
{
    size_t table_index;
    uint32_t size;
};

struct anf_atom_pointer_deref
{
    anf_atom_var pointer;
    type_id assigned_type = ILLEGAL_TYPE;
};

using anf_atom = std::variant<
    anf_atom_var,
    anf_atom_number,
    anf_atom_bool,
    anf_atom_char,
    anf_atom_string,
    anf_atom_pointer_deref
>;

struct anf_binary_expression
{
    anf_binary_op operation;
    anf_atom left;
    anf_atom right;
};

struct anf_cast_expression
{
    anf_atom value;
    type_id target_type = ILLEGAL_TYPE;
};

struct anf_call_expression
{
    std::string function_name;
    symbol_id function_symbol = ILLEGAL_SYMBOL;
    std::vector<anf_atom> arguments;
    type_id result_type = ILLEGAL_TYPE;
};

struct anf_record_get_expression
{
    anf_atom object;
    size_t field_index = 0;
    std::string debug_field_name;
    type_id field_type = ILLEGAL_TYPE;
};

using anf_let_value = std::variant<
    anf_atom,
    anf_binary_expression,
    anf_cast_expression,
    anf_call_expression,
    anf_record_get_expression
>;

struct anf_let_statement
{
    anf_binding target;
    anf_let_value value;
};

struct anf_record_set_statement
{
    anf_atom object;
    size_t field_index = 0;
    std::string debug_field_name;
    anf_atom value;
};

struct anf_assign_statement
{
    anf_binding target;
    anf_atom value;
};

struct anf_call_statement
{
    std::string function_name;
    symbol_id function_symbol = ILLEGAL_SYMBOL;
    std::vector<anf_atom> arguments;
};

struct anf_return_statement
{
    std::optional<anf_atom> value;
};

struct anf_block;
struct anf_if_statement
{
    anf_atom condition;
    std::unique_ptr<anf_block> then_block;
    std::unique_ptr<anf_block> else_block;
};

struct anf_while_statement
{
    std::unique_ptr<anf_block> condition_setup;
    anf_atom condition;
    std::unique_ptr<anf_block> body;
};

using anf_statement = std::variant<
    anf_let_statement,
    anf_record_set_statement,
    anf_assign_statement,
    anf_call_statement,
    anf_return_statement,
    anf_if_statement,
    anf_while_statement
>;

struct anf_block
{
    std::vector<anf_statement> statements;
};

struct anf_import_definition
{
    std::string ns_name;
    std::string function_name;
    function_signature signature;
    std::optional<std::string> alias;
};

struct anf_global_definition
{
    std::string name;
    symbol_id symbol_ref = ILLEGAL_SYMBOL;
    type_id assigned_type = ILLEGAL_TYPE;
    int32_t init_value = 0;
};

struct anf_function_definition
{
    std::vector<capy_attribute> attributes;
    anf_binding function;
    function_signature signature;
    std::unique_ptr<anf_block> body;
};

struct anf_module
{
    std::vector<anf_import_definition> imports;
    std::vector<anf_global_definition> globals;
    std::vector<anf_function_definition> functions;
};

std::optional<anf_binary_op> to_anf_binary_op(operator_type op);
void dump_anf_module(const context& ctx, const anf_module& module, size_t indent = 0);

class anf_generator : private diagnostic_emitter
{
public:
    explicit anf_generator(diagnostic_bag& diagnostics, context& ctx);
    anf_module generate(node_module& module);

private:
    void lower_module(const node_module& n);
    void lower_import(const node_import_definition& n);
    void lower_global(const node_global_definition& n);
    void lower_function(source_range location, const node_function_definition& n);

    bool lower_statement(const node_expr& node, anf_block& block);
    void lower_block(const std::vector<std::unique_ptr<node_expr>>& expressions, anf_block& block);
    std::optional<anf_atom> lower_atom(const node_expr& node) const;
    std::optional<anf_atom> lower_atomized(const node_expr& node, anf_block& block);
    std::optional<anf_let_value> lower_let_value(const node_expr& node, anf_block& block);
    std::optional<anf_atom> lower_condition(const node_expr& node, anf_block& block);
    std::optional<std::pair<std::unique_ptr<anf_block>, anf_atom>> lower_while_condition(const node_expr& node);
    std::optional<size_t> record_field_index(type_id record_type_id, const std::string& field_name) const;

    anf_binding binding_from_symbol(symbol_id symbol_ref, const std::string& fallback_name, type_id explicit_type = ILLEGAL_TYPE) const;
    std::string next_temp_name();

private:
    context& parse_context;
    anf_module generated_module;
    uint32_t temp_index = 0;
};
