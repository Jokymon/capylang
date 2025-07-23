#include "parser.hpp"
#include <ostream>

class emitter
{
public:
    explicit emitter(std::ostream &output);

    void generate(const ast_node& node);

    void emit(const ast_node& node);

    void emit(const node_module& module_def);
    void emit(const node_import_definition& import_def);
    void emit(const node_function_head& function_head);
    void emit(const node_function_definition& func_def);
    void emit(const node_function_call& func_call);
    void emit(const node_expression& root);
    void emit(const node_var_reference& variable);
    void emit(const node_number& number);

private:
    void emit_function_signature(const std::string& function_name, const function_signature& signature);

    std::ostream &output_;
};