#pragma once
#include "ast.hpp"
#include "lexer.hpp"
#include "symbol.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

struct lir_located_node
{
    source_range location;
};

struct lir_attribute
{
    std::string name;
};

using lir_attribute_bag = std::vector<lir_attribute>;

struct lir_attributed_node
{
    lir_attribute_bag attributes;
    bool has_attribute(const std::string& attr_name) const;
};

enum class lir_binary_op
{
    multiply,
    division,
    modulus,
    plus,
    minus,

    equals,
    notequals,
    lessthan,
    lessthan_equal,
    greaterthan,
    greaterthan_equal,

    and_op,
    or_op,
    shl,
    shr
};

struct lir_base_var
{
    std::string name;
    symbol_id symbol_ref;
};

struct lir_deref
{
};
struct lir_field
{
    size_t index;
};

// either a deref, field, index, ....
using lir_place_elem = std::variant<
    lir_deref,
    lir_field
>;

struct lir_place
{
    // TODO: if we base all places on variables, then we explicitly
    // exclude expression based pointer dereference. Is this on purpose?
    lir_base_var base;
    std::vector<lir_place_elem> projection;
};

struct lir_base
{
};

struct lir_number;
struct lir_char_literal;
struct lir_bool_literal;
struct lir_string_literal;
struct lir_record_init;
struct lir_load_expression;
struct lir_store_expression;
struct lir_if_expression;
struct lir_while_expression;
struct lir_function_call;
struct lir_cast_expression;
struct lir_discard_expression;
struct lir_return_expression;
struct lir_break_statement;
struct lir_unary_expression;
struct lir_binary_expression;

using lir_node_raw = std::variant<
    lir_number,
    lir_char_literal,
    lir_bool_literal,
    lir_string_literal,
    lir_record_init,
    lir_load_expression,
    lir_store_expression,
    lir_if_expression,
    lir_while_expression,
    lir_function_call,
    lir_cast_expression,
    lir_discard_expression,
    lir_return_expression,
    lir_break_statement,
    lir_unary_expression,
    lir_binary_expression
>;

using lir_node = located<lir_node_raw>;
using lir_node_list = std::vector<std::unique_ptr<lir_node>>;

// We are using X-Macros here (https://en.wikipedia.org/wiki/X_macro) to simplify the definition of LIR-node structs and
// their related dump-functions. Sadly, compile time reflection in C++26 Clang is still not where we would need it for
// this kind of functionality, sorry :-(
#define DEFINE_NODES
#include "dumpable.hpp"
#include "lir_nodes.hpp"
#undef DEFINE_NODES

// ----------------------------------------------------------

type_id lir_assigned_node_type(const lir_node& node, context& ctx);

class lir_generator
{
public:
    explicit lir_generator(context& ctx);

    lir_module generate(const node_module& module_def);

private:
    lir_node_list generate(const node_number& node);
    lir_node_list generate(const node_char_literal& node);
    lir_node_list generate(const node_bool_literal& node);
    lir_node_list generate(const node_string_literal& node);
    lir_node_list generate(const node_var_reference& node);
    lir_node_list generate(const node_pointer_deref& node);
    lir_node_list generate(const node_let_expression& node);
    lir_node_list generate(const node_if_expression& node);
    lir_node_list generate(const node_while_expression& node);
    lir_node_list generate(const node_record_definition& node);
    lir_node_list generate(const node_record_initialisation& node);
    lir_node_list generate(const node_field_deref& node);
    lir_node_list generate(const node_function_call& node);
    lir_node_list generate(const node_cast_expression& node);
    lir_node_list generate(const node_discard_expression& node);
    lir_node_list generate(const node_return_expression& node);
    lir_node_list generate(const node_unary_expression& node);
    lir_node_list generate(const node_binary_expression& node);
    lir_node_list generate(const node_break_statement& node);
    lir_node_list generate(const node_expr& expression);

    std::unique_ptr<lir_import_definition> generate(const node_import_definition& imp_def);
    std::unique_ptr<lir_global_definition> generate(const node_global_definition& global_def);
    std::unique_ptr<lir_function_definition> generate(const node_function_definition& func_def);

private:
    context& parse_context;
};

// ----------------------------------------------------------

void dump_lir(std::ostream& os, const context& ctx, const lir_module& module_def, int indent = 0);
void dump_lir(std::ostream& os, const context& ctx, const lir_node& node, int indent = 0);