#include "parser.hpp"
#include <ostream>

class emitter
{
public:
    explicit emitter(std::ostream &output);

    void generate(ast_node& node);

    void emit(ast_node& node);

    void emit(node_module& module_def);
    void emit(const node_import_definition& import_def);
    void emit(const node_function_head& function_head);
    void emit(const node_function_definition& func_def);
    void emit(const node_function_call& func_call);
    void emit(const node_let_expression& let_expression);
    void emit(const node_record_initialisation& record_init);
    void emit(const node_field_deref& field_deref);
    void emit(const node_expression& root);
    void emit(const node_var_reference& variable);
    void emit(const node_pointer_deref& ptr_deref);
    void emit(const node_string_literal& literal);
    void emit(const node_number& number);

private:
    void emit_function_signature(const std::string& function_name, const function_signature& signature);
    uint32_t allocate_data(const std::string& data);

    std::ostream &output_;
    std::string data_buffer;
    uint32_t data_offset;
};