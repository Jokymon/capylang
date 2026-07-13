#include "lir.hpp"
#include "lowering_strategy.hpp"
#include <ostream>

class wasm_block;
class wasm_data_section;
class wasm_module;

class lir_based_emitter
{
public:
    explicit lir_based_emitter(context& ctx);
    ~lir_based_emitter();

    wasm_module generate(lir::module& module_def);

    void emit(lir::expr& node);
    void emit(lir::statement& node);

    void emit(const lir::import_definition& import_def);
    void emit(const lir::global_definition& global_def);
    void emit(const lir::function_definition& func_def);

    void emit(const lir::while_statement& while_stmt);
    void emit(const lir::store_statement& load);
    void emit(const lir::store_string_statement& load);
    void emit(const lir::store_record_statement& record_init);
    void emit(const lir::expression_statement& root);
    void emit(const lir::break_statement& root);
    void emit(const lir::return_statement& root);

    void emit(const lir::binary_expression& root);
    void emit(const lir::unary_expression& root);
    void emit(const lir::cast_expression& root);
    void emit(const lir::function_call& func_call);
    void emit(const lir::if_expression& if_expr);
    void emit(const lir::load_expression& load);
    void emit(const lir::allocate_record_expression& root);
    void emit(const lir::string_literal& literal);
    void emit(const lir::bool_literal& number);
    void emit(const lir::char_literal& literal);
    void emit(const lir::number& number);

private:
    uint32_t allocate_data(const std::string& data);

    context& parse_context;
    std::unique_ptr<lowering_strategy> lowering;

    lir::module* current_module;

    wasm_data_section* cur_data;
    wasm_module* cur_mod;
    wasm_block* cur_block;
};
