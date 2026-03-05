#include "parser.hpp"
#include <ostream>

class wasm_block;
class wasm_data_section;
class wasm_module;

class emitter
{
public:
    explicit emitter(context& ctx);
    ~emitter();

    wasm_module generate(node_module& module_def);

    void emit(ast_node& node);

    void emit(const node_import_definition& import_def);
    void emit(const node_global_definition& global_def);
    void emit(const node_function_definition& func_def);
    void emit(const node_record_definition& record_def);
    void emit(const node_if_expression& if_expr);
    void emit(const node_while_expression& if_expr);
    void emit(const node_function_call& func_call);
    void emit(const node_let_expression& let_expression);
    void emit(const node_record_initialisation& record_init);
    void emit(const node_field_deref& field_deref);
    void emit(const node_cast_expression& root);
    void emit(const node_discard_expression& root);
    void emit(const node_return_expression& root);
    void emit(const node_expression& root);
    void emit(const node_var_reference& variable);
    void emit(const node_pointer_deref& ptr_deref);
    void emit(const node_string_literal& literal);
    void emit(const node_char_literal& literal);
    void emit(const node_bool_const& number);
    void emit(const node_number& number);

private:
    uint32_t allocate_data(const std::string& data);

    context& parse_context;

    node_module* current_module;

    wasm_data_section* cur_data;
    wasm_module* cur_mod;
    wasm_block* cur_block;
};
