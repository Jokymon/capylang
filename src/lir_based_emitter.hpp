#pragma once

#include "compile_options.hpp"
#include "lir.hpp"
#include "wasm.hpp"

class lir_based_emitter
{
public:
    explicit lir_based_emitter(compile_options options, context& ctx);
    ~lir_based_emitter();

    wasm::module generate(lir::module& module_def);

    void emit(lir::expr& node);
    void emit(lir::statement& node);

    void emit(const lir::import_definition& import_def);
    void emit(const lir::global_definition& global_def);
    void emit(const lir::function_definition& func_def);

    void emit(const lir::while_statement& while_stmt);
    void emit(const lir::store_statement& store);
    void emit(const lir::store_string_statement& store);
    void emit(const lir::store_record_statement& record_init);
    void emit(const lir::expression_statement& stmt);
    void emit(const lir::break_statement& stmt);
    void emit(const lir::return_statement& stmt);

    void emit(const lir::binary_expression& expr);
    void emit(const lir::unary_expression& expr);
    void emit(const lir::cast_expression& expr);
    void emit(const lir::function_call& func_call);
    void emit(const lir::if_expression& if_expr);
    void emit(const lir::load_expression& load);
    void emit(const lir::allocate_record_expression& expr);
    void emit(const lir::string_literal& literal);
    void emit(const lir::bool_literal& literal);
    void emit(const lir::char_literal& literal);
    void emit(const lir::number& number);

private:
    std::unique_ptr<wasm::memory> create_memory(size_t initial_block_count);
    std::unique_ptr<wasm::function> create_function(const std::string& name, wasm::type_t return_type, std::vector<wasm::argument> arguments);
    std::unique_ptr<wasm::global> create_global(const std::string& name, wasm::type_t type, wasm::access_t access);
    void export_as(const std::string& export_name, const wasm::exportable& object);

    void lower_function_arguments(const lir::function_head& func_head, std::vector<wasm::argument>& arguments);
    void lower_function_argument(const type_kind& type, const std::string& basename, std::vector<wasm::argument>& arguments);
    void lower_local_variable(const symbol& sym, wasm::function& function);
    void lower_local_type(const type_kind& type, const std::string& basename, wasm::function& function);
    wasm::type_t lower_type(type_id type) const;
    wasm::type_t value_type_of(type_id type) const;

    uint32_t allocate_data(const std::string& data);
    std::string next_label(const char* prefix);

private:
    compile_options options;
    context& parse_context;

    std::unique_ptr<wasm::data_section> cur_data;
    wasm::module* cur_mod = nullptr;
    wasm::instruction_list* cur_instructions = nullptr;
    const wasm::branch_label* cur_loop_exit = nullptr;
    size_t label_index = 0;
};
