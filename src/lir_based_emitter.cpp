#include "lir_based_emitter.hpp"

#include "tools.hpp"
#include "wasm_builder.hpp"

#include <algorithm>
#include <variant>

namespace
{
    size_t type_size(const context& ctx, type_id type)
    {
        const auto& type_entry = ctx.types[to_index(type)];

        return std::visit(
            [&](const auto& entry) -> size_t
            {
                using T = std::decay_t<decltype(entry)>;
                if constexpr (std::is_same_v<T, type_kind>)
                {
                    return std::visit(
                        [&](const auto& kind) -> size_t
                        {
                            using K = std::decay_t<decltype(kind)>;
                            if constexpr (std::is_same_v<K, primitive_type>)
                            {
                                switch (kind)
                                {
                                    case primitive_type::Void:
                                        return 0;
                                    case primitive_type::U8:
                                    case primitive_type::S8:
                                        return 1;
                                    case primitive_type::U16:
                                    case primitive_type::S16:
                                        return 2;
                                    case primitive_type::U32:
                                    case primitive_type::S32:
                                    case primitive_type::Char:
                                    case primitive_type::Boolean:
                                        return 4;
                                    case primitive_type::U64:
                                    case primitive_type::S64:
                                        return 8;
                                }
                            }
                            else if constexpr (std::is_same_v<K, pointer_type>)
                            {
                                return 4;
                            }
                            else if constexpr (std::is_same_v<K, record_type>)
                            {
                                size_t size = 0;
                                for (const auto& field : kind.fields)
                                {
                                    size += type_size(ctx, field.second);
                                }
                                return size;
                            }
                            return 0;
                        },
                        entry
                    );
                }
                else
                {
                    CAPY_ASSERT(entry.parent.has_value(), "Encountered unresolved type variable when getting type size");
                    return type_size(ctx, entry.parent.value());
                }
            },
            type_entry
        );
    }

    wasm::type_t lower_wasm_type(const context& ctx, type_id type)
    {
        type_id resolved_type = ctx.resolved_type(type);
        const auto& type_entry = ctx.types[to_index(resolved_type)];
        CAPY_ASSERT(std::holds_alternative<type_kind>(type_entry), "Found unresolved type in emitter");

        return std::visit(
            [](const auto& kind) -> wasm::type_t
            {
                using K = std::decay_t<decltype(kind)>;
                if constexpr (std::is_same_v<K, primitive_type>)
                {
                    switch (kind)
                    {
                        case primitive_type::Void:
                            return wasm::type_t::None;
                        case primitive_type::Char:
                        case primitive_type::S8:
                            return wasm::type_t::I8;
                        case primitive_type::S16:
                            return wasm::type_t::I16;
                        case primitive_type::S32:
                            return wasm::type_t::I32;
                        case primitive_type::S64:
                            return wasm::type_t::I64;
                        case primitive_type::U8:
                            return wasm::type_t::U8;
                        case primitive_type::U16:
                            return wasm::type_t::U16;
                        case primitive_type::U32:
                            return wasm::type_t::U32;
                        case primitive_type::U64:
                            return wasm::type_t::U64;
                        case primitive_type::Boolean:
                            return wasm::type_t::I32;
                    }
                }
                else if constexpr (std::is_same_v<K, pointer_type> || std::is_same_v<K, record_type>)
                {
                    return wasm::type_t::I32;
                }

                CAPY_FAIL("Unknown type kind in emitter type lowering");
                return wasm::type_t::None;
            },
            std::get<type_kind>(type_entry)
        );
    }

    wasm::type_t lower_memory_type(const context& ctx, type_id type)
    {
        type_id resolved_type = ctx.resolved_type(type);
        const auto& type_entry = ctx.types[to_index(resolved_type)];
        CAPY_ASSERT(std::holds_alternative<type_kind>(type_entry), "Found unresolved memory type in emitter");

        const auto* primitive = std::get_if<primitive_type>(&std::get<type_kind>(type_entry));
        if (primitive == nullptr)
            return wasm::type_t::I32;

        switch (*primitive)
        {
            case primitive_type::S8:
                return wasm::type_t::I8;
            case primitive_type::S16:
                return wasm::type_t::I16;
            case primitive_type::S64:
                return wasm::type_t::I64;
            case primitive_type::U8:
                return wasm::type_t::U8;
            case primitive_type::U16:
                return wasm::type_t::U16;
            case primitive_type::U64:
                return wasm::type_t::U64;
            default:
                return wasm::type_t::I32;
        }
    }

    type_id assigned_node_type(const lir::expr& node, context& ctx)
    {
        return std::visit(
            [&](const auto& value) -> type_id
            {
                using T = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<T, lir::number>)
                    return value.assigned_type;
                else if constexpr (std::is_same_v<T, lir::char_literal>)
                    return ctx.intern_primitive(primitive_type::Char);
                else if constexpr (std::is_same_v<T, lir::bool_literal>)
                    return ctx.intern_primitive(primitive_type::Boolean);
                else if constexpr (std::is_same_v<T, lir::string_literal>)
                    return ctx.BUILTIN_STRING;
                else if constexpr (std::is_same_v<T, lir::load_expression>)
                    return value.assigned_type;
                else if constexpr (std::is_same_v<T, lir::if_expression>)
                    return value.assigned_type;
                else if constexpr (std::is_same_v<T, lir::cast_expression>)
                    return value.cast_type;
                else if constexpr (std::is_same_v<T, lir::unary_expression>)
                    return value.assigned_type;
                else if constexpr (std::is_same_v<T, lir::binary_expression>)
                    return value.assigned_type;
                else if constexpr (std::is_same_v<T, lir::allocate_record_expression>)
                    return value.type;
                else if constexpr (std::is_same_v<T, lir::function_call>)
                {
                    const auto& sym = ctx.symbol_at(value.symbol_ref);
                    CAPY_ASSERT(sym.kind == symbol_kind::function, "LIR function call should resolve to a function symbol");
                    auto return_type = ctx.function_return_type(sym.signature.function_type);
                    CAPY_ASSERT(return_type.has_value(), "LIR function call should have a valid return type");
                    return return_type.value();
                }
                else
                {
                    CAPY_FAIL("Compiler error: unhandled LIR expression type; index=%lu", node.index());
                    return ILLEGAL_TYPE;
                }
            },
            node
        );
    }

    struct sl_argument
    {
        std::string name;
        type_id type;
    };

    struct sl_local_var
    {
        std::string name;
        type_id type;
    };

    struct sl_global_var
    {
        std::string name;
        type_id type;
    };

    struct sl_ptr
    {
        size_t offset;
        type_id type;
    };

    using storage_loc = std::variant<sl_argument, sl_local_var, sl_global_var, sl_ptr>;

    storage_loc transform_symbol(const symbol& sym)
    {
        switch (sym.kind)
        {
            case symbol_kind::argument:
                return sl_argument{sym.name, sym.symbol_type};
            case symbol_kind::local_var:
                return sl_local_var{sym.name, sym.symbol_type};
            case symbol_kind::global_var:
                return sl_global_var{sym.name, sym.symbol_type};
            default:
                CAPY_FAIL("Can't transform symbol with symbol_kind %u", static_cast<unsigned int>(sym.kind));
                return sl_local_var{};
        }
    }

    storage_loc transform_place(const context& ctx, const storage_loc& storage, const lir::place_elem& place, wasm::instruction_list& instructions)
    {
        return std::visit(
            overloaded{
                [&](const lir::deref&) -> storage_loc
                {
                    type_id deref_type{0};
                    std::visit(
                        overloaded{
                            [&](const sl_argument& arg)
                            {
                                wasm_builder::local_get(instructions, arg.name);
                                deref_type = ctx.derefed_type(arg.type).value();
                            },
                            [&](const sl_local_var& local)
                            {
                                wasm_builder::local_get(instructions, local.name);
                                deref_type = ctx.derefed_type(local.type).value();
                            },
                            [&](const sl_global_var& global)
                            {
                                wasm_builder::global_get(instructions, global.name);
                                deref_type = ctx.derefed_type(global.type).value();
                            },
                            [&](const sl_ptr& ptr)
                            {
                                wasm_builder::load(instructions, wasm::type_t::I32, ptr.offset);
                                deref_type = ctx.derefed_type(ptr.type).value();
                            }
                        },
                        storage
                    );
                    return sl_ptr{0, deref_type};
                },
                [&](const lir::field& field) -> storage_loc
                {
                    return std::visit(
                        overloaded{
                            [&](const sl_argument& arg) -> storage_loc
                            {
                                const auto& type_spec = ctx.types[to_index(arg.type)];
                                auto* record = get_type_from_node<record_type>(type_spec);
                                CAPY_ASSERT(record != nullptr, "Trying to access a field of a non-record type");
                                CAPY_ASSERT(field.index < record->fields.size(), "Record field index out of bounds");
                                return sl_argument{arg.name + "_" + std::to_string(field.index), record->fields[field.index].second};
                            },
                            [&](const sl_local_var& local) -> storage_loc
                            {
                                auto record_type_id = ctx.record_behind(local.type);
                                CAPY_ASSERT(record_type_id.has_value(), "Trying to access a field of a non-record type");
                                const auto& type_spec = ctx.types[to_index(record_type_id.value())];
                                auto* record = get_type_from_node<record_type>(type_spec);
                                CAPY_ASSERT(record != nullptr, "Expected a concrete record type");
                                CAPY_ASSERT(field.index < record->fields.size(), "Record field index out of bounds");
                                return sl_local_var{local.name + "_" + std::to_string(field.index), record->fields[field.index].second};
                            },
                            [&](const sl_global_var& global) -> storage_loc
                            {
                                const auto& type_spec = ctx.types[to_index(global.type)];
                                auto* record = get_type_from_node<record_type>(type_spec);
                                CAPY_ASSERT(record != nullptr, "Trying to access a field of a non-record type");
                                CAPY_ASSERT(field.index < record->fields.size(), "Record field index out of bounds");
                                return sl_global_var{global.name + "_" + std::to_string(field.index), record->fields[field.index].second};
                            },
                            [&](const sl_ptr& ptr) -> storage_loc
                            {
                                const auto& type_spec = ctx.types[to_index(ptr.type)];
                                auto* record = get_type_from_node<record_type>(type_spec);
                                CAPY_ASSERT(record != nullptr, "Trying to access a field of a non-record type");
                                CAPY_ASSERT(field.index < record->fields.size(), "Record field index out of bounds");

                                size_t field_offset = 0;
                                for (size_t index = 0; index < field.index; index++)
                                {
                                    field_offset += type_size(ctx, record->fields[index].second);
                                }
                                return sl_ptr{ptr.offset + field_offset, record->fields[field.index].second};
                            }
                        },
                        storage
                    );
                }
            },
            place
        );
    }

    void transform_load(const context& ctx, const storage_loc& storage, wasm::instruction_list& instructions)
    {
        std::visit(
            overloaded{
                [&](const sl_argument& arg)
                {
                    if (ctx.is_record_type(arg.type))
                    {
                        const auto& type_spec = ctx.types[to_index(arg.type)];
                        auto* record = get_type_from_node<record_type>(type_spec);
                        for (size_t index = 0; index < record->fields.size(); index++)
                        {
                            wasm_builder::local_get(instructions, arg.name + "_" + std::to_string(index));
                        }
                    }
                    else
                        wasm_builder::local_get(instructions, arg.name);
                },
                [&](const sl_local_var& local)
                {
                    if (ctx.is_record_type(local.type))
                    {
                        const auto& type_spec = ctx.types[to_index(local.type)];
                        auto* record = get_type_from_node<record_type>(type_spec);
                        for (size_t index = 0; index < record->fields.size(); index++)
                        {
                            wasm_builder::local_get(instructions, local.name + "_" + std::to_string(index));
                        }
                    }
                    else
                        wasm_builder::local_get(instructions, local.name);
                },
                [&](const sl_global_var& global)
                {
                    if (ctx.is_record_type(global.type))
                    {
                        const auto& type_spec = ctx.types[to_index(global.type)];
                        auto* record = get_type_from_node<record_type>(type_spec);
                        for (size_t index = 0; index < record->fields.size(); index++)
                        {
                            wasm_builder::global_get(instructions, global.name + "_" + std::to_string(index));
                        }
                    }
                    else
                        wasm_builder::global_get(instructions, global.name);
                },
                [&](const sl_ptr& ptr)
                {
                    wasm_builder::load(instructions, lower_memory_type(ctx, ptr.type), ptr.offset);
                }
            },
            storage
        );
    }

    void transform_store(const context& ctx, const storage_loc& storage, wasm::instruction_list& instructions)
    {
        std::visit(
            overloaded{
                [&](const sl_argument& arg)
                {
                    if (ctx.is_record_type(arg.type))
                    {
                        const auto& type_spec = ctx.types[to_index(arg.type)];
                        auto* record = get_type_from_node<record_type>(type_spec);
                        for (size_t index = 0; index < record->fields.size(); index++)
                        {
                            wasm_builder::local_set(instructions, arg.name + "_" + std::to_string(index));
                        }
                    }
                    else
                        wasm_builder::local_set(instructions, arg.name);
                },
                [&](const sl_local_var& local)
                {
                    if (ctx.is_record_type(local.type))
                    {
                        const auto& type_spec = ctx.types[to_index(local.type)];
                        auto* record = get_type_from_node<record_type>(type_spec);
                        for (size_t index = 0; index < record->fields.size(); index++)
                        {
                            wasm_builder::local_set(instructions, local.name + "_" + std::to_string(index));
                        }
                    }
                    else
                        wasm_builder::local_set(instructions, local.name);
                },
                [&](const sl_global_var& global)
                {
                    if (ctx.is_record_type(global.type))
                    {
                        const auto& type_spec = ctx.types[to_index(global.type)];
                        auto* record = get_type_from_node<record_type>(type_spec);
                        for (size_t index = 0; index < record->fields.size(); index++)
                        {
                            wasm_builder::global_set(instructions, global.name + "_" + std::to_string(index));
                        }
                    }
                    else
                        wasm_builder::global_set(instructions, global.name);
                },
                [&](const sl_ptr& ptr)
                {
                    wasm_builder::store(instructions, lower_memory_type(ctx, ptr.type), ptr.offset);
                }
            },
            storage
        );
    }
}

lir_based_emitter::lir_based_emitter(compile_options options, context& ctx)
: options(options)
, parse_context(ctx)
, cur_data(std::make_unique<wasm::data_section>())
{
    cur_data->init_offset = 100;
    cur_data->offset = 100;
    allocate_data(std::string("\x42\x00\x00\x00\x10\x00\x00\x00\x10\x20\x30\x40Test", 16));
}

lir_based_emitter::~lir_based_emitter() = default;

wasm::module lir_based_emitter::generate(lir::module& module_def)
{
    wasm::module ir_module;
    cur_mod = &ir_module;

    auto memory = create_memory(options.mem_min_blocks);
    if (options.mem_max_blocks.has_value())
        memory->max_block_count = options.mem_max_blocks.value();
    if ((memory_type::exported == options.mem_type) || (memory_type::impex == options.mem_type))
    {
        memory->name = "memory";
        export_as("memory", *memory);
    }
    if ((memory_type::imported == options.mem_type) || (memory_type::impex == options.mem_type))
    {
        memory->ns = "env";
        memory->import_name = "memory";
    }
    ir_module.memories.push_back(std::move(memory));

    for (auto& literal : parse_context.string_literals)
    {
        literal.start_address = allocate_data(literal.literal);
    }

    for (const auto& import_def : module_def.imports)
        emit(*import_def);
    for (const auto& global_def : module_def.globals)
        emit(*global_def);
    for (const auto& func_def : module_def.functions)
        emit(*func_def);

    ir_module.data_sections.push_back(std::move(cur_data));
    cur_mod = nullptr;
    return ir_module;
}

void lir_based_emitter::emit(lir::expr& node)
{
    std::visit([this](auto& value)
               { emit(value); },
               node);
}

void lir_based_emitter::emit(lir::statement& node)
{
    std::visit([this](auto& value)
               { emit(value); },
               node);
}

void lir_based_emitter::emit(const lir::import_definition& import_def)
{
    std::vector<wasm::argument> arguments;
    lower_function_arguments(import_def.function_head, arguments);

    const std::string& imported_name = import_def.function_head.name;
    std::string internal_name = import_def.alias.value_or(imported_name);
    auto return_type = parse_context.function_return_type(import_def.function_head.signature.function_type);
    CAPY_ASSERT(return_type.has_value(), "Imported function should have a resolved return type");

    auto function = create_function(internal_name, lower_type(return_type.value()), std::move(arguments));
    function->ns = import_def.ns_name;
    function->import_name = imported_name;
    cur_mod->functions.push_back(std::move(function));
}

void lir_based_emitter::emit(const lir::global_definition& global_def)
{
    const auto& global_symbol = parse_context.symbol_at(global_def.symbol_ref);
    auto access = global_symbol.mutab ? wasm::access_t::Mut : wasm::access_t::Immut;
    auto global = create_global(global_symbol.name, lower_type(global_symbol.symbol_type), access);

    auto* previous_instructions = cur_instructions;
    cur_instructions = &global->init_expression;
    emit(*global_def.init_expression);
    cur_instructions = previous_instructions;

    cur_mod->globals.push_back(std::move(global));
}

void lir_based_emitter::emit(const lir::function_definition& func_def)
{
    std::vector<wasm::argument> arguments;
    lower_function_arguments(func_def.head, arguments);

    auto return_type = parse_context.function_return_type(func_def.head.signature.function_type);
    CAPY_ASSERT(return_type.has_value(), "Function should have a resolved return type");
    auto function = create_function(func_def.head.name, lower_type(return_type.value()), std::move(arguments));

    if (std::find(func_def.attributes.begin(), func_def.attributes.end(), "export") != func_def.attributes.end())
    {
        export_as(func_def.head.name, *function);
    }

    for (const auto& [identifier, symbol_id] : func_def.function_scope->symbol_table)
    {
        const auto& sym = parse_context.symbol_at(symbol_id);
        if (sym.kind == symbol_kind::local_var)
            lower_local_variable(sym, *function);
    }

    cur_instructions = &function->function_body;
    for (const auto& statement : func_def.code)
        emit(*statement);
    cur_instructions = nullptr;

    cur_mod->functions.push_back(std::move(function));
}

void lir_based_emitter::emit(const lir::while_statement& while_stmt)
{
    auto* previous_instructions = cur_instructions;
    auto* previous_loop_exit = cur_loop_exit;

    auto& exit_block = wasm_builder::block(*cur_instructions, wasm::type_t::None, next_label("while_exit"));
    auto& loop_block = wasm_builder::loop(exit_block.instructions, wasm::type_t::None, next_label("while"));
    cur_loop_exit = exit_block.block_label.get();
    cur_instructions = &loop_block.instructions;

    emit(*while_stmt.condition);
    wasm_builder::eqz(*cur_instructions, wasm::type_t::I32);
    wasm_builder::br_if(*cur_instructions, *exit_block.block_label);
    for (const auto& statement : while_stmt.while_code)
        emit(*statement);
    wasm_builder::br(*cur_instructions, *loop_block.block_label);

    cur_instructions = previous_instructions;
    cur_loop_exit = previous_loop_exit;
}

void lir_based_emitter::emit(const lir::store_statement& store)
{
    const auto& target = parse_context.symbol_at(store.target.base.symbol_ref);
    storage_loc storage = transform_symbol(target);
    for (const auto& projection : store.target.projection)
    {
        storage = transform_place(parse_context, storage, projection, *cur_instructions);
    }

    emit(*store.value);
    transform_store(parse_context, storage, *cur_instructions);
}

void lir_based_emitter::emit(const lir::store_string_statement& store)
{
    const auto& target = parse_context.symbol_at(store.target.base.symbol_ref);
    uint32_t ptr = parse_context.string_literals[store.string_table_index].start_address;
    wasm_builder::const_val(*cur_instructions, wasm::type_t::I32, ptr);
    wasm_builder::const_val(*cur_instructions, wasm::type_t::I32, store.string_size);

    if (target.kind == symbol_kind::global_var)
    {
        wasm_builder::global_set(*cur_instructions, target.name + "_1");
        wasm_builder::global_set(*cur_instructions, target.name + "_0");
    }
    else
    {
        CAPY_ASSERT(target.kind == symbol_kind::local_var || target.kind == symbol_kind::argument, "Unexpected string storage target");
        wasm_builder::local_set(*cur_instructions, target.name + "_1");
        wasm_builder::local_set(*cur_instructions, target.name + "_0");
    }
}

void lir_based_emitter::emit(const lir::store_record_statement&)
{
    CAPY_FAIL("The LIR emitter doesn't expect store record expressions after lowering");
}

void lir_based_emitter::emit(const lir::expression_statement& stmt)
{
    emit(*stmt.expression);
    if (stmt.is_dropped && !parse_context.is_primitive_type(assigned_node_type(*stmt.expression, parse_context), primitive_type::Void))
    {
        wasm_builder::drop(*cur_instructions);
    }
}

void lir_based_emitter::emit(const lir::break_statement&)
{
    CAPY_ASSERT(cur_loop_exit != nullptr, "Trying to break from outside a loop");
    wasm_builder::br(*cur_instructions, *cur_loop_exit);
}

void lir_based_emitter::emit(const lir::return_statement& stmt)
{
    if (stmt.expression)
        emit(*stmt.expression);
    if (stmt.is_explicit)
        wasm_builder::ret(*cur_instructions);
}

void lir_based_emitter::emit(const lir::binary_expression& expr)
{
    emit(*expr.left);
    emit(*expr.right);

    wasm::type_t operand_type = value_type_of(assigned_node_type(*expr.left, parse_context));

    switch (expr.op)
    {
        case lir::binary_op_t::Minus:
            wasm_builder::sub(*cur_instructions, operand_type);
            break;
        case lir::binary_op_t::Plus:
            wasm_builder::add(*cur_instructions, operand_type);
            break;
        case lir::binary_op_t::Multiply:
            wasm_builder::mul(*cur_instructions, operand_type);
            break;
        case lir::binary_op_t::Division:
            wasm_builder::div(*cur_instructions, operand_type);
            break;
        case lir::binary_op_t::Modulus:
            wasm_builder::mod(*cur_instructions, operand_type);
            break;
        case lir::binary_op_t::AndOp:
            wasm_builder::and_instr(*cur_instructions, operand_type);
            break;
        case lir::binary_op_t::OrOp:
            wasm_builder::or_instr(*cur_instructions, operand_type);
            break;
        case lir::binary_op_t::Shl:
            wasm_builder::shl(*cur_instructions, operand_type);
            break;
        case lir::binary_op_t::Shr:
            wasm_builder::shr(*cur_instructions, operand_type);
            break;
        case lir::binary_op_t::Equals:
            wasm_builder::eq(*cur_instructions, operand_type);
            break;
        case lir::binary_op_t::NotEquals:
            wasm_builder::ne(*cur_instructions, operand_type);
            break;
        case lir::binary_op_t::LessThan:
            wasm_builder::lt(*cur_instructions, operand_type);
            break;
        case lir::binary_op_t::LessThanEqual:
            wasm_builder::lte(*cur_instructions, operand_type);
            break;
        case lir::binary_op_t::GreaterThan:
            wasm_builder::gt(*cur_instructions, operand_type);
            break;
        case lir::binary_op_t::GreaterThanEqual:
            wasm_builder::gte(*cur_instructions, operand_type);
            break;
    }
}

void lir_based_emitter::emit(const lir::unary_expression& expr)
{
    wasm::type_t operand_type = value_type_of(assigned_node_type(*expr.expr, parse_context));
    wasm_builder::const_val(*cur_instructions, operand_type, 0);
    emit(*expr.expr);
    wasm_builder::sub(*cur_instructions, operand_type);
}

void lir_based_emitter::emit(const lir::cast_expression& expr)
{
    emit(*expr.expression);

    auto is_64bit = [](wasm::type_t type)
    { return type == wasm::type_t::I64 || type == wasm::type_t::U64; };
    wasm::type_t source_type = value_type_of(assigned_node_type(*expr.expression, parse_context));
    wasm::type_t target_type = value_type_of(expr.cast_type);

    // TODO: Handle the remaining s8/s16 conversions, including sign extension
    // when widening within the i32 stack representation.
    if (!is_64bit(source_type) && is_64bit(target_type))
    {
        bool source_is_signed = source_type == wasm::type_t::I8 ||
                                source_type == wasm::type_t::I16 ||
                                source_type == wasm::type_t::I32;
        if (source_is_signed)
            wasm_builder::i64_extend_i32_s(*cur_instructions);
        else
            wasm_builder::i64_extend_i32_u(*cur_instructions);
    }
    else if (is_64bit(source_type) && !is_64bit(target_type) && target_type != wasm::type_t::None)
    {
        wasm_builder::i32_wrap_i64(*cur_instructions);
    }

    if (parse_context.is_primitive_type(expr.cast_type, primitive_type::Void) &&
        !parse_context.is_primitive_type(assigned_node_type(*expr.expression, parse_context), primitive_type::Void))
    {
        wasm_builder::drop(*cur_instructions);
    }
}

void lir_based_emitter::emit(const lir::function_call& func_call)
{
    for (const auto& argument : func_call.arguments)
        emit(*argument);

    const auto& function_symbol = parse_context.symbol_at(func_call.symbol_ref);
    if (!function_symbol.is_intrinsic)
    {
        wasm_builder::call(*cur_instructions, func_call.function_name);
        return;
    }

    if (function_symbol.name == "memory_size")
        wasm_builder::memory_size(*cur_instructions);
    else if (function_symbol.name == "memory_grow")
        wasm_builder::memory_grow(*cur_instructions);
    else if (function_symbol.name == "memory_fill")
        wasm_builder::memory_fill(*cur_instructions);
    else if (function_symbol.name == "unreachable")
        wasm_builder::unreachable(*cur_instructions);
    else
    {
        CAPY_FAIL(
            "Unknown intrinsic: symbol='%s', call='%s'",
            function_symbol.name.c_str(),
            func_call.function_name.c_str()
        );
    }
}

void lir_based_emitter::emit(const lir::if_expression& if_expr)
{
    auto* previous_instructions = cur_instructions;
    emit(*if_expr.condition);
    auto& if_block = wasm_builder::if_block(*cur_instructions, lower_type(if_expr.assigned_type), next_label("if"));

    cur_instructions = &if_block.then_block;
    for (const auto& statement : if_expr.then_code)
        emit(*statement);

    cur_instructions = &if_block.else_block;
    for (const auto& statement : if_expr.else_code)
        emit(*statement);

    cur_instructions = previous_instructions;
}

void lir_based_emitter::emit(const lir::load_expression& load)
{
    const auto& source = parse_context.symbol_at(load.source.base.symbol_ref);
    storage_loc storage = transform_symbol(source);
    for (const auto& projection : load.source.projection)
    {
        storage = transform_place(parse_context, storage, projection, *cur_instructions);
    }
    transform_load(parse_context, storage, *cur_instructions);
}

void lir_based_emitter::emit(const lir::allocate_record_expression& expr)
{
    auto record_type_id = parse_context.record_behind(expr.type);
    CAPY_ASSERT(record_type_id.has_value(), "Expected a record or pointer-to-record type");

    wasm_builder::const_val(*cur_instructions, wasm::type_t::I32, 0);
    wasm_builder::const_val(*cur_instructions, wasm::type_t::I32, 0);
    wasm_builder::const_val(*cur_instructions, wasm::type_t::I32, 4);
    wasm_builder::const_val(*cur_instructions, wasm::type_t::I32, type_size(parse_context, record_type_id.value()));
    wasm_builder::call(*cur_instructions, "cabi_realloc");
}

void lir_based_emitter::emit(const lir::string_literal& literal)
{
    uint32_t ptr = parse_context.string_literals[literal.table_index].start_address;
    wasm_builder::const_val(*cur_instructions, wasm::type_t::I32, ptr);
    wasm_builder::const_val(*cur_instructions, wasm::type_t::I32, literal.size);
}

void lir_based_emitter::emit(const lir::bool_literal& literal)
{
    wasm_builder::const_val(*cur_instructions, wasm::type_t::I32, literal.value ? 1 : 0);
}

void lir_based_emitter::emit(const lir::char_literal& literal)
{
    wasm_builder::const_val(*cur_instructions, wasm::type_t::I32, literal.ch);
}

void lir_based_emitter::emit(const lir::number& number)
{
    wasm_builder::const_val(*cur_instructions, value_type_of(number.assigned_type), number.value);
}

std::unique_ptr<wasm::memory> lir_based_emitter::create_memory(size_t initial_block_count)
{
    auto memory = std::make_unique<wasm::memory>();
    memory->initial_block_count = initial_block_count;
    memory->max_block_count = 0;
    memory->index = cur_mod->memories.size();
    memory->export_type = wasm::extern_index_t::MemIdx;
    return memory;
}

std::unique_ptr<wasm::function> lir_based_emitter::create_function(
    const std::string& name,
    wasm::type_t return_type,
    std::vector<wasm::argument> arguments
)
{
    auto function = std::make_unique<wasm::function>();
    function->name = name;
    function->index = cur_mod->functions.size();
    function->export_type = wasm::extern_index_t::FuncIdx;
    function->arguments = std::move(arguments);
    function->return_type = return_type;
    return function;
}

std::unique_ptr<wasm::global> lir_based_emitter::create_global(
    const std::string& name,
    wasm::type_t type,
    wasm::access_t access
)
{
    auto global = std::make_unique<wasm::global>();
    global->name = name;
    global->typ = type;
    global->access = access;
    return global;
}

void lir_based_emitter::export_as(const std::string& export_name, const wasm::exportable& object)
{
    auto export_item = std::make_unique<wasm::export_item>();
    export_item->export_name = export_name;
    export_item->name = object.name;
    export_item->index = object.index;
    export_item->export_type = object.export_type;
    cur_mod->exports.push_back(std::move(export_item));
}

void lir_based_emitter::lower_function_arguments(
    const lir::function_head& func_head,
    std::vector<wasm::argument>& arguments
)
{
    const auto& type_entry = parse_context.types[to_index(func_head.signature.function_type)];
    const auto& function = std::get<function_type>(std::get<type_kind>(type_entry));
    const auto& parameters = func_head.signature.parameters;
    CAPY_ASSERT(parameters.size() == function.parameter_types.size(), "Function parameter name/type count mismatch");

    for (size_t index = 0; index < parameters.size(); index++)
    {
        type_id type = parse_context.resolved_type(function.parameter_types[index]);
        CAPY_ASSERT(parse_context.is_resolved(type), "Function argument type should be resolved in emitter stage");
        const auto& parameter_entry = parse_context.types[to_index(type)];
        CAPY_ASSERT(std::holds_alternative<type_kind>(parameter_entry), "Unexpected type variable in resolved function argument");
        lower_function_argument(std::get<type_kind>(parameter_entry), parameters[index].name, arguments);
    }
}

void lir_based_emitter::lower_function_argument(
    const type_kind& type,
    const std::string& basename,
    std::vector<wasm::argument>& arguments
)
{
    std::visit(
        [&](const auto& kind)
        {
            using K = std::decay_t<decltype(kind)>;
            if constexpr (std::is_same_v<K, primitive_type>)
            {
                switch (kind)
                {
                    case primitive_type::Void:
                        arguments.push_back({basename, wasm::type_t::None});
                        break;
                    case primitive_type::Char:
                    case primitive_type::S8:
                        arguments.push_back({basename, wasm::type_t::I8});
                        break;
                    case primitive_type::S16:
                        arguments.push_back({basename, wasm::type_t::I16});
                        break;
                    case primitive_type::S32:
                        arguments.push_back({basename, wasm::type_t::I32});
                        break;
                    case primitive_type::S64:
                        arguments.push_back({basename, wasm::type_t::I64});
                        break;
                    case primitive_type::U8:
                        arguments.push_back({basename, wasm::type_t::U8});
                        break;
                    case primitive_type::U16:
                        arguments.push_back({basename, wasm::type_t::U16});
                        break;
                    case primitive_type::U32:
                        arguments.push_back({basename, wasm::type_t::U32});
                        break;
                    case primitive_type::U64:
                        arguments.push_back({basename, wasm::type_t::U64});
                        break;
                    case primitive_type::Boolean:
                        arguments.push_back({basename, wasm::type_t::I32});
                        break;
                }
            }
            else if constexpr (std::is_same_v<K, pointer_type>)
            {
                arguments.push_back({basename, wasm::type_t::I32});
            }
            else if constexpr (std::is_same_v<K, record_type>)
            {
                for (size_t index = 0; index < kind.fields.size(); index++)
                {
                    type_id field_type = parse_context.resolved_type(kind.fields[index].second);
                    CAPY_ASSERT(parse_context.is_resolved(field_type), "Record field type should be resolved in emitter stage");
                    const auto& field_entry = parse_context.types[to_index(field_type)];
                    CAPY_ASSERT(std::holds_alternative<type_kind>(field_entry), "Unexpected type variable in resolved record field");
                    lower_function_argument(std::get<type_kind>(field_entry), basename + "_" + std::to_string(index), arguments);
                }
            }
            else
            {
                CAPY_FAIL("Unhandled type in function argument lowering");
            }
        },
        type
    );
}

void lir_based_emitter::lower_local_variable(const symbol& sym, wasm::function& function)
{
    type_id variable_type = parse_context.resolved_type(sym.symbol_type);
    const auto& type_entry = parse_context.types[to_index(variable_type)];
    CAPY_ASSERT(std::holds_alternative<type_kind>(type_entry), "Encountered unresolved type variable when lowering local variable");
    lower_local_type(std::get<type_kind>(type_entry), sym.name, function);
}

void lir_based_emitter::lower_local_type(
    const type_kind& type,
    const std::string& basename,
    wasm::function& function
)
{
    std::visit(
        [&](const auto& kind)
        {
            using K = std::decay_t<decltype(kind)>;
            if constexpr (std::is_same_v<K, primitive_type>)
            {
                wasm::type_t type = wasm::type_t::None;
                switch (kind)
                {
                    case primitive_type::Void:
                        type = wasm::type_t::None;
                        break;
                    case primitive_type::Char:
                    case primitive_type::S8:
                        type = wasm::type_t::I8;
                        break;
                    case primitive_type::S16:
                        type = wasm::type_t::I16;
                        break;
                    case primitive_type::S32:
                        type = wasm::type_t::I32;
                        break;
                    case primitive_type::S64:
                        type = wasm::type_t::I64;
                        break;
                    case primitive_type::U8:
                        type = wasm::type_t::U8;
                        break;
                    case primitive_type::U16:
                        type = wasm::type_t::U16;
                        break;
                    case primitive_type::U32:
                        type = wasm::type_t::U32;
                        break;
                    case primitive_type::U64:
                        type = wasm::type_t::U64;
                        break;
                    case primitive_type::Boolean:
                        type = wasm::type_t::I32;
                        break;
                }
                function.locals.push_back({basename, type});
            }
            else if constexpr (std::is_same_v<K, pointer_type>)
            {
                function.locals.push_back({basename, wasm::type_t::I32});
            }
            else if constexpr (std::is_same_v<K, record_type>)
            {
                for (size_t index = 0; index < kind.fields.size(); index++)
                {
                    type_id field_type = parse_context.resolved_type(kind.fields[index].second);
                    CAPY_ASSERT(parse_context.is_resolved(field_type), "Record field type should be resolved in emitter stage");
                    const auto& field_entry = parse_context.types[to_index(field_type)];
                    CAPY_ASSERT(std::holds_alternative<type_kind>(field_entry), "Unexpected type variable in resolved record field");
                    lower_local_type(std::get<type_kind>(field_entry), basename + "_" + std::to_string(index), function);
                }
            }
            else
            {
                CAPY_FAIL("Unknown type kind when lowering local variable");
            }
        },
        type
    );
}

wasm::type_t lir_based_emitter::lower_type(type_id type) const
{
    return lower_wasm_type(parse_context, type);
}

wasm::type_t lir_based_emitter::value_type_of(type_id type) const
{
    type_id resolved = parse_context.resolved_type(type);
    if (!parse_context.is_resolved(resolved))
        return wasm::type_t::I32;
    return lower_type(resolved);
}

uint32_t lir_based_emitter::allocate_data(const std::string& data)
{
    auto allocation_offset = cur_data->offset;
    cur_data->offset += data.size();
    cur_data->data_buffer += data;
    return static_cast<uint32_t>(allocation_offset);
}

std::string lir_based_emitter::next_label(const char* prefix)
{
    return std::string(prefix) + "_" + std::to_string(label_index++);
}
