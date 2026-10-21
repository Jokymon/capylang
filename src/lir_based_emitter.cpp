#include "lir_based_emitter.hpp"
#include "lower_cabi.hpp"
#include "symbol.hpp"
#include "tools.hpp"
#include <ranges>
#include <variant>

size_t lir_type_size(const context& ctx, type_id idx)
{
    const auto& type_entry = ctx.types[to_index(idx)];

    return std::visit(
        [&](const auto& t) -> size_t
        {
            using T = std::decay_t<decltype(t)>;

            if constexpr (std::is_same_v<T, type_kind>)
            {
                auto type_spec = t;

                return std::visit([&](const auto& k) -> size_t
                                  {
                    using K = std::decay_t<decltype(k)>;

                    if constexpr (std::is_same_v<K, primitive_type>) {
                        switch (k) {
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
                        CAPY_FAIL("Unknown primitive type in lir_type_size");
                        return 0;
                    } else if constexpr (std::is_same_v<K, pointer_type>) {
                        return 4;
                    } else if constexpr (std::is_same_v<K, record_type>) {
                        size_t size = 0;
                        for (const auto& field : k.fields) {
                            size += lir_type_size(ctx, field.second);
                        }
                        return size;
                    } else {
                        return 0;
                    } },
                                  type_spec);
            }
            else if constexpr (std::is_same_v<T, type_var>)
            {
                if (t.parent.has_value())
                {
                    return lir_type_size(ctx, t.parent.value());
                }
                else
                {
                    CAPY_FAIL("Encountered unresolved type variable in when getting type size");
                    return 0;
                }
            }
        },
        type_entry
    );
}

type_id lir_assigned_node_type(const lir::expr& node, context& ctx)
{
    return std::visit(
        [&](const auto& n) -> type_id
        {
            using T = std::decay_t<decltype(n)>;

            if constexpr (std::is_same_v<T, lir::number>)
            {
                return n.assigned_type;
            }
            else if constexpr (std::is_same_v<T, lir::char_literal>)
            {
                return ctx.intern_primitive(primitive_type::Char);
            }
            else if constexpr (std::is_same_v<T, lir::bool_literal>)
            {
                return ctx.intern_primitive(primitive_type::Boolean);
            }
            else if constexpr (std::is_same_v<T, lir::string_literal>)
            {
                return ctx.BUILTIN_STRING;
            }
            else if constexpr (std::is_same_v<T, lir::load_expression>)
            {
                return n.assigned_type;
            }
            else if constexpr (std::is_same_v<T, lir::if_expression>)
            {
                return n.assigned_type;
            }
            else if constexpr (std::is_same_v<T, lir::function_call>)
            {
                const auto& sym = ctx.symbol_at(n.symbol_ref);
                CAPY_ASSERT(sym.kind == symbol_kind::function, "Compiler Error: LIR function call should resolve to function symbol");

                auto return_type = ctx.function_return_type(sym.signature.function_type);
                CAPY_ASSERT(return_type.has_value(), "Compiler Error: LIR function call type should have a valid return type");
                return return_type.value();
            }
            else if constexpr (std::is_same_v<T, lir::cast_expression>)
            {
                return n.cast_type;
            }
            else if constexpr (std::is_same_v<T, lir::unary_expression>)
            {
                return n.assigned_type;
            }
            else if constexpr (std::is_same_v<T, lir::binary_expression>)
            {
                return n.assigned_type;
            }
            else
            {
                CAPY_FAIL("Compiler error: LIR encountered unhandled type; index=%lu", node.index());
                return ILLEGAL_TYPE;
            }
        },
        node
    );
}

// A storage_loc represents a storge location for one step in the variable plus
// projections list of a lir_place. It is used in the process of resolving and
// lowering a lir_place object.
struct sl_argument;
struct sl_local_var;
struct sl_global_var;
struct sl_ptr;
using storage_loc = std::variant<sl_argument, sl_local_var, sl_global_var, sl_ptr>;

// An sl_argument represents a value that is stored in one or multiple local
// WASM variables
struct sl_argument
{
    std::string name;
    type_id type;
};

// An sl_local_var represents a value that is stored in one or multiple local
// WASM variables. It is listed separatly from sl_argument since function
// arguments can potentially be lowered differently than local variables
struct sl_local_var
{
    std::string name;
    type_id type;
};

// An sl_global_var represents a value that is stored in a single
// global WASM variable.
struct sl_global_var
{
    std::string name;
    type_id type;
};

// An sl_ptr represents a combination of a pointer value that is currently on
// the WASM data stack and an offset value that needs to be used for any load
// or store operation in WASM.
struct sl_ptr
{
    size_t offset;
    type_id type; // The base type of the value to which this pointer points
};

storage_loc transform_symbol(const context& parse_context, const symbol& sym)
{
    switch (sym.kind)
    {
        case symbol_kind::argument:
            return sl_argument{
                .name = sym.name,
                .type = sym.symbol_type
            };
        case symbol_kind::local_var:
            return sl_local_var{
                .name = sym.name,
                .type = sym.symbol_type
            };
        case symbol_kind::global_var:
            return sl_global_var{
                .name = sym.name,
                .type = sym.symbol_type
            };
        default:
            CAPY_FAIL("Can't transform symbol with symbol_kind %u", static_cast<unsigned int>(sym.kind));
    }
}

storage_loc transform_place(const context& parse_context, const storage_loc& storage, const lir::place_elem& place, wasm_expr& code_block)
{
    return place.visit(
        overloaded{
            [&](const lir::deref&) -> storage_loc
            {
                type_id deref_type = type_id{0};

                storage.visit(
                    overloaded{
                        [&](const sl_argument& arg)
                        {
                            wasm_builder::local_get(code_block, arg.name.c_str());
                            // TODO: These .value() are actually ugly and we should make
                            // sure that they are really resolved
                            deref_type = parse_context.derefed_type(arg.type).value();
                        },
                        [&](const sl_local_var& local)
                        {
                            wasm_builder::local_get(code_block, local.name.c_str());
                            deref_type = parse_context.derefed_type(local.type).value();
                        },
                        [&](const sl_global_var& global)
                        {
                            wasm_builder::global_get(code_block, global.name.c_str());
                            deref_type = parse_context.derefed_type(global.type).value();
                        },
                        [&](const sl_ptr& ptr)
                        {
                            // The type for loading is always fixed here, because we know
                            // that it is a pointer; TODO: maybe introduce a ptr-type in
                            // the WASM IR.
                            // The offset is taken from the given sl_ptr because we need
                            // to include this in our load here.
                            wasm_builder::load(code_block, wasm_type::i32, ptr.offset);
                            deref_type = parse_context.derefed_type(ptr.type).value();
                        }
                    }
                );
                return sl_ptr{0, deref_type};
            },
            [&](const lir::field& f) -> storage_loc
            {
                return storage.visit(
                    overloaded{
                        [&](const sl_argument& arg) -> storage_loc
                        {
                            const auto& rec_type_spec = parse_context.types[to_index(arg.type)];
                            auto* rec_type = get_type_from_node<record_type>(rec_type_spec);
                            CAPY_ASSERT(rec_type != nullptr, "Trying to access a field of an non-record type");

                            return sl_argument{
                                .name = arg.name + "_" + std::to_string(f.index),
                                .type = rec_type->fields[f.index].second
                            };
                        },
                        [&](const sl_local_var& local) -> storage_loc
                        {
                            // TODO: check again if the record_behind is really necessary;
                            // with proper normalisation and intermediate lowering steps, we
                            // should actually have a pointer to a record which is handled
                            // straight forward and we can just assume that we are dealing
                            // we a record here
                            auto rec_type = parse_context.record_behind(local.type);
                            CAPY_ASSERT(rec_type.has_value(), "Trying to access a field of an non-record type");
                            const auto& type_spec = parse_context.types[to_index(rec_type.value())];
                            auto* rec_type_spec = get_type_from_node<record_type>(type_spec);
                            CAPY_ASSERT(rec_type_spec != nullptr, "Fatal error while assume that we are dealing with a record");

                            return sl_local_var{
                                .name = local.name + "_" + std::to_string(f.index),
                                .type = rec_type_spec->fields[f.index].second
                            };
                        },
                        [&](const sl_global_var& global) -> storage_loc
                        {
                            const auto& rec_type_spec = parse_context.types[to_index(global.type)];
                            auto* rec_type = get_type_from_node<record_type>(rec_type_spec);
                            CAPY_ASSERT(rec_type != nullptr, "Trying to access a field of an non-record type");

                            return sl_global_var{
                                .name = global.name + "_" + std::to_string(f.index),
                                .type = rec_type->fields[f.index].second
                            };
                        },
                        [&](const sl_ptr& ptr) -> storage_loc
                        {
                            const auto& rec_type_spec = parse_context.types[to_index(ptr.type)];
                            auto* rec_type = get_type_from_node<record_type>(rec_type_spec);
                            CAPY_ASSERT(rec_type != nullptr, "Trying to access a field of an non-record type");

                            size_t field_offset = 0;
                            size_t field_index = 0;
                            for (const auto& field_definition : rec_type->fields)
                            {
                                if (field_index == f.index)
                                    break;
                                field_offset += lir_type_size(parse_context, field_definition.second);
                                field_index++;
                            }

                            return sl_ptr{
                                .offset = ptr.offset + field_offset,
                                .type = rec_type->fields[f.index].second
                            };
                        }
                    }
                );
            }
        }
    );
}

void transform_load(const context& parse_context, const storage_loc& storage, wasm_expr& code_block)
{
    storage.visit(
        overloaded{
            [&](const sl_argument& arg)
            {
                if (parse_context.is_record_type(arg.type))
                {
                    const auto& rec_type_spec = parse_context.types[to_index(arg.type)];
                    auto* rec_type = get_type_from_node<record_type>(rec_type_spec);
                    size_t field_index = 0;
                    for (const auto& field_definition : rec_type->fields)
                    {
                        wasm_builder::local_get(code_block, (arg.name + "_" + std::to_string(field_index)).c_str());
                        field_index++;
                    }
                }
                else
                {
                    wasm_builder::local_get(code_block, arg.name.c_str());
                }
            },
            [&](const sl_local_var& local)
            {
                if (parse_context.is_record_type(local.type))
                {
                    const auto& rec_type_spec = parse_context.types[to_index(local.type)];
                    auto* rec_type = get_type_from_node<record_type>(rec_type_spec);
                    size_t field_index = 0;
                    for (const auto& field_definition : rec_type->fields)
                    {
                        wasm_builder::local_get(code_block, (local.name + "_" + std::to_string(field_index)).c_str());
                        field_index++;
                    }
                }
                else
                {
                    wasm_builder::local_get(code_block, local.name.c_str());
                }
            },
            [&](const sl_global_var& global)
            {
                if (parse_context.is_record_type(global.type))
                {
                    const auto& rec_type_spec = parse_context.types[to_index(global.type)];
                    auto* rec_type = get_type_from_node<record_type>(rec_type_spec);
                    size_t field_index = 0;
                    for (const auto& field_definition : rec_type->fields)
                    {
                        wasm_builder::global_get(code_block, (global.name + "_" + std::to_string(field_index)).c_str());
                        field_index++;
                    }
                }
                else
                {
                    wasm_builder::global_get(code_block, global.name.c_str());
                }
            },
            [&](const sl_ptr& ptr)
            {
                if (parse_context.is_primitive_type(ptr.type, primitive_type::U8))
                {
                    // TODO: we should simplify this by using a mapping from
                    // primitive types to wasm_types
                    wasm_builder::load(code_block, wasm_type::u8, ptr.offset);
                }
                else if (parse_context.is_primitive_type(ptr.type, primitive_type::U16))
                {
                    wasm_builder::load(code_block, wasm_type::u16, ptr.offset);
                }
                else
                {
                    wasm_builder::load(code_block, wasm_type::i32, ptr.offset);
                }
            }
        }
    );
}

void transform_store(const context& parse_context, const storage_loc& storage, wasm_expr& code_block)
{
    storage.visit(
        overloaded{
            [&](const sl_argument& arg)
            {
                if (parse_context.is_record_type(arg.type))
                {
                    const auto& rec_type_spec = parse_context.types[to_index(arg.type)];
                    auto* rec_type = get_type_from_node<record_type>(rec_type_spec);
                    size_t field_index = 0;
                    for (const auto& field_definition : rec_type->fields)
                    {
                        // TODO: do we need to reverse the field order here?
                        wasm_builder::local_set(code_block, (arg.name + "_" + std::to_string(field_index)).c_str());
                        field_index++;
                    }
                }
                else
                {
                    wasm_builder::local_set(code_block, arg.name.c_str());
                }
            },
            [&](const sl_local_var& local)
            {
                if (parse_context.is_record_type(local.type))
                {
                    const auto& rec_type_spec = parse_context.types[to_index(local.type)];
                    auto* rec_type = get_type_from_node<record_type>(rec_type_spec);
                    size_t field_index = 0;
                    for (const auto& field_definition : rec_type->fields)
                    {
                        // TODO: do we need to reverse the field order here?
                        wasm_builder::local_set(code_block, (local.name + "_" + std::to_string(field_index)).c_str());
                        field_index++;
                    }
                }
                else
                {
                    wasm_builder::local_set(code_block, local.name.c_str());
                }
            },
            [&](const sl_global_var& global)
            {
                if (parse_context.is_record_type(global.type))
                {
                    const auto& rec_type_spec = parse_context.types[to_index(global.type)];
                    auto* rec_type = get_type_from_node<record_type>(rec_type_spec);
                    size_t field_index = 0;
                    for (const auto& field_definition : rec_type->fields)
                    {
                        // TODO: do we need to reverse the field order here?
                        wasm_builder::global_set(code_block, (global.name + "_" + std::to_string(field_index)).c_str());
                        field_index++;
                    }
                }
                else
                {
                    wasm_builder::global_set(code_block, global.name.c_str());
                }
            },
            [&](const sl_ptr& ptr)
            {
                if (parse_context.is_primitive_type(ptr.type, primitive_type::U8))
                {
                    // TODO: we should simplify this by using a mapping from
                    // primitive types to wasm_types
                    wasm_builder::store(code_block, wasm_type::u8, ptr.offset);
                }
                else if (parse_context.is_primitive_type(ptr.type, primitive_type::U16))
                {
                    wasm_builder::store(code_block, wasm_type::u16, ptr.offset);
                }
                else
                {
                    wasm_builder::store(code_block, wasm_type::i32, ptr.offset);
                }
            }
        }
    );
}

lir_based_emitter::lir_based_emitter(compile_options options, context& ctx)
: options(options)
, parse_context(ctx)
, lowering(new lower_cabi(ctx))
, cur_loop_block(nullptr)
, cur_control_block(nullptr)
, cur_expr(nullptr)
{
    cur_data = new wasm_data_section(100);
    allocate_data(std::string("\x42\x00\x00\x00\x10\x00\x00\x00\x10\x20\x30\x40Test", 16));
}

lir_based_emitter::~lir_based_emitter()
{
    delete cur_data;
}

wasm_module lir_based_emitter::generate(lir::module& module_def)
{
    wasm_module ir_module;
    cur_mod = &ir_module;

    current_module = &module_def;

    auto& memory = ir_module.create_memory(options.mem_min_blocks);
    if (options.mem_max_blocks.has_value())
    {
        memory.max_block_count = options.mem_max_blocks.value();
    }

    if ((memory_type::exported == options.mem_type) || (memory_type::impex == options.mem_type))
    {
        ir_module.export_as("memory", memory);
    }
    if ((memory_type::imported == options.mem_type) || (memory_type::impex == options.mem_type))
    {
        memory.import_from("env", "memory");
    }

    for (auto& literal : parse_context.string_literals)
    {
        literal.start_address = allocate_data(literal.literal);
    }

    for (const auto& import_def : module_def.imports)
    {
        emit(*import_def);
    }

    for (const auto& global_def : module_def.globals)
    {
        emit(*global_def);
    }

    for (const auto& func_def : module_def.functions)
    {
        emit(*func_def);
    }

    cur_mod->append_data_section(*cur_data);

    current_module = nullptr;
    cur_mod = nullptr;

    return ir_module;
}

void lir_based_emitter::emit(lir::expr& node)
{
    std::visit(
        [this](auto& n)
        {
            this->emit(n);
        },
        node
    );
}

void lir_based_emitter::emit(lir::statement& node)
{
    std::visit(
        [this](auto& n)
        {
            this->emit(n);
        },
        node
    );
}

void lir_based_emitter::emit(const lir::import_definition& import_def)
{
    arguments_type args;
    auto function_name = import_def.function_head.name;

    lowering->lower_function_arguments(import_def.function_head, args);

    std::string imported_function_name = import_def.function_head.name;
    std::string internal_function_name = imported_function_name;

    if (import_def.alias.has_value())
    {
        internal_function_name = import_def.alias.value();
    }

    auto return_type = parse_context.function_return_type(import_def.function_head.signature.function_type).value();
    auto& import_func = cur_mod->create_function(internal_function_name.c_str(), lowering->lower_return_type(return_type), args);
    import_func.import_from(import_def.ns_name.c_str(), imported_function_name.c_str());
}

void lir_based_emitter::emit(const lir::global_definition& global_def)
{
    const auto& global_symbol = parse_context.symbol_at(global_def.symbol_ref);
    auto mutability = global_symbol.mutab ? access_type::mut : access_type::immut;
    auto global_type = lowering->lower_return_type(parse_context.resolved_type(global_symbol.symbol_type));
    auto& new_global = cur_mod->create_global(global_symbol.name.c_str(), global_type, mutability);

    cur_expr = &new_global.init_expression;
    emit(*global_def.init_expression);
    cur_expr = nullptr;
}

void lir_based_emitter::emit(const lir::function_definition& func_def)
{
    arguments_type args;
    auto function_name = func_def.head.name;

    lowering->lower_function_arguments(func_def.head, args);

    auto return_type = parse_context.function_return_type(func_def.head.signature.function_type).value();
    auto& func = cur_mod->create_function(function_name.c_str(), lowering->lower_return_type(return_type), args);
    if (std::find(func_def.attributes.begin(), func_def.attributes.end(), "export") != func_def.attributes.end())
    {
        cur_mod->export_as(function_name.c_str(), func);
    }
    cur_expr = &func.function_body;

    for (const auto& [identifier, symbol_id] : func_def.function_scope->symbol_table)
    {
        const auto& sym = parse_context.symbol_at(symbol_id);
        if (sym.kind == symbol_kind::local_var)
        {
            lowering->lower_local_variable(sym, func);
        }
    }

    for (const auto& expression : func_def.code)
    {
        emit(*expression);
    }
}

void lir_based_emitter::emit(const lir::while_statement& while_stmt)
{
    wasm_block* prev_loop_block = cur_loop_block;
    wasm_control_block* prev_control_block = cur_control_block;
    wasm_expr* prev_expr = cur_expr;

    wasm_block& while_exit = wasm_builder::block(*cur_expr, cur_control_block, wasm_type::none);
    wasm_block& while_block = wasm_builder::loop(while_exit.instructions, &while_exit, wasm_type::none);
    cur_loop_block = &while_block;
    cur_control_block = &while_block;
    cur_expr = &while_block.instructions;

    emit(*while_stmt.condition);

    wasm_builder::eqz(*cur_expr, wasm_type::i32);
    wasm_builder::br_if(*cur_expr, while_exit.label());

    for (const auto& expression : while_stmt.while_code)
    {
        emit(*expression);
    }
    wasm_builder::br(*cur_expr, while_block.label());

    cur_loop_block = prev_loop_block;
    cur_control_block = prev_control_block;
    cur_expr = prev_expr;
}

void lir_based_emitter::emit(const lir::store_statement& store)
{
    const auto& target_var = parse_context.symbol_at(store.target.base.symbol_ref);

    storage_loc sl = transform_symbol(parse_context, target_var);
    for (const auto& elem : store.target.projection)
    {
        sl = transform_place(parse_context, sl, elem, *cur_expr);
    }

    emit(*store.value);

    transform_store(parse_context, sl, *cur_expr);
}

void lir_based_emitter::emit(const lir::store_string_statement& store)
{
    const auto& target_sym = parse_context.symbol_at(store.target.base.symbol_ref);

    uint32_t ptr = parse_context.string_literals[store.string_table_index].start_address;
    wasm_builder::const_val(*cur_expr, wasm_type::i32, ptr);
    wasm_builder::const_val(*cur_expr, wasm_type::i32, store.string_size);

    if (target_sym.kind == symbol_kind::global_var)
    {
        wasm_builder::global_set(*cur_expr, (target_sym.name + "_1").c_str());
        wasm_builder::global_set(*cur_expr, (target_sym.name + "_0").c_str());
    }
    else if (target_sym.kind == symbol_kind::local_var)
    {
        wasm_builder::local_set(*cur_expr, (target_sym.name + "_1").c_str());
        wasm_builder::local_set(*cur_expr, (target_sym.name + "_0").c_str());
    }
}

void lir_based_emitter::emit(const lir::store_record_statement& record_init)
{
    CAPY_FAIL("The LIR emitter doesn't expect any store record expressions anymore\n");
}

void lir_based_emitter::emit(const lir::expression_statement& stmt)
{
    emit(*stmt.expression);
    if (stmt.is_dropped && !parse_context.is_primitive_type(lir_assigned_node_type(*stmt.expression, parse_context), primitive_type::Void))
    {
        wasm_builder::drop(*cur_expr);
    }
}

void lir_based_emitter::emit(const lir::break_statement& root)
{
    // cur_loop_block tracks the innermost loop independently from the complete
    // control-block chain, which may also contain if blocks.
    if (cur_loop_block != nullptr && cur_loop_block->block_type == wasm_block::t_loop)
    {
        // TODO: This feels a little hacky; we assume that a loop is always
        // enclosed in a plain 'block' because it is constructed from a while
        // loop. This however may not always be the case in the future and we
        // should instead make sure, we really target the right block for
        // branch instead of heuristically guessing it.
        wasm_builder::br(*cur_expr, cur_loop_block->enclosing_block->label());
    }
    else
    {
        CAPY_FAIL("Trying to break from inside non-loop block; this should already have been caught by semantic check");
    }
}

void lir_based_emitter::emit(const lir::return_statement& root)
{
    if (root.expression)
    {
        emit(*root.expression);
    }
    if (root.is_explicit)
    {
        wasm_builder::ret(*cur_expr);
    }
}

void lir_based_emitter::emit(const lir::binary_expression& root)
{
    emit(*root.left);
    emit(*root.right);

    // Both operands have the same type (ensured by type inference and the
    // semantic analysis). The operand type decides about the width (i32/i64)
    // and the signedness of the instruction; for comparisons the result is
    // always a bool, so root.assigned_type can't be used here.
    wasm_type operand_type = value_type_of(lir_assigned_node_type(*root.left, parse_context));

    switch (root.op)
    {
        case lir::binary_op_t::Minus:
            wasm_builder::sub(*cur_expr, operand_type);
            break;
        case lir::binary_op_t::Plus:
            wasm_builder::add(*cur_expr, operand_type);
            break;
        case lir::binary_op_t::Multiply:
            wasm_builder::mul(*cur_expr, operand_type);
            break;
        case lir::binary_op_t::Division:
            wasm_builder::div(*cur_expr, operand_type);
            break;
        case lir::binary_op_t::Modulus:
            wasm_builder::mod(*cur_expr, operand_type);
            break;
        case lir::binary_op_t::AndOp:
            wasm_builder::and_instr(*cur_expr, operand_type);
            break;
        case lir::binary_op_t::OrOp:
            wasm_builder::or_instr(*cur_expr, operand_type);
            break;
        case lir::binary_op_t::Shl:
            wasm_builder::shl(*cur_expr, operand_type);
            break;
        case lir::binary_op_t::Shr:
            wasm_builder::shr(*cur_expr, operand_type);
            break;
        case lir::binary_op_t::Equals:
            wasm_builder::eq(*cur_expr, operand_type);
            break;
        case lir::binary_op_t::NotEquals:
            wasm_builder::ne(*cur_expr, operand_type);
            break;
        case lir::binary_op_t::LessThan:
            wasm_builder::lt(*cur_expr, operand_type);
            break;
        case lir::binary_op_t::LessThanEqual:
            wasm_builder::lte(*cur_expr, operand_type);
            break;
        case lir::binary_op_t::GreaterThan:
            wasm_builder::gt(*cur_expr, operand_type);
            break;
        case lir::binary_op_t::GreaterThanEqual:
            wasm_builder::gte(*cur_expr, operand_type);
            break;
    }
}

void lir_based_emitter::emit(const lir::unary_expression& root)
{
    // negation is emitted as '0 - expr' in the type of the expression
    wasm_type operand_type = value_type_of(lir_assigned_node_type(*root.expr, parse_context));
    wasm_builder::const_val(*cur_expr, operand_type, 0);
    emit(*root.expr);
    wasm_builder::sub(*cur_expr, operand_type);
}

void lir_based_emitter::emit(const lir::cast_expression& root)
{
    emit(*root.expression);

    // Casts between 32-bit and 64-bit integers need a conversion instruction;
    // all types up to 32 bits are i32 values in WASM and need none.
    auto is_64bit = [](wasm_type t)
    { return (t == wasm_type::i64) || (t == wasm_type::u64); };
    wasm_type source_type = value_type_of(lir_assigned_node_type(*root.expression, parse_context));
    wasm_type target_type = value_type_of(root.cast_type);
    if (!is_64bit(source_type) && is_64bit(target_type))
    {
        // the signedness of the source decides how the upper bits are filled
        bool source_is_signed = (source_type == wasm_type::i8) || (source_type == wasm_type::i16) ||
                                (source_type == wasm_type::i32);
        if (source_is_signed)
        {
            wasm_builder::i64_extend_i32_s(*cur_expr);
        }
        else
        {
            wasm_builder::i64_extend_i32_u(*cur_expr);
        }
    }
    else if (is_64bit(source_type) && !is_64bit(target_type) && (target_type != wasm_type::none))
    {
        wasm_builder::i32_wrap_i64(*cur_expr);
    }
    if ((parse_context.is_primitive_type(root.cast_type, primitive_type::Void)) &&
        (!parse_context.is_primitive_type(lir_assigned_node_type(*root.expression, parse_context), primitive_type::Void)))
    {
        wasm_builder::drop(*cur_expr);
    }
}

void lir_based_emitter::emit(const lir::function_call& func_call)
{
    for (const auto& argument : func_call.arguments)
    {
        // TODO: add ABI-related adaptations in the lowering here
        emit(*argument);
    }

    const auto& func_symbol = parse_context.symbol_at(func_call.symbol_ref);
    if (func_symbol.is_intrinsic)
    {
        if (func_symbol.name == "memory_size")
        {
            wasm_builder::memory_size(*cur_expr);
        }
        else if (func_symbol.name == "memory_grow")
        {
            wasm_builder::memory_grow(*cur_expr);
        }
        else if (func_symbol.name == "memory_fill")
        {
            wasm_builder::memory_fill(*cur_expr);
        }
        else if (func_symbol.name == "unreachable")
        {
            wasm_builder::unreachable(*cur_expr);
        }
        else
        {
            CAPY_FAIL("Unknown intrinsic");
        }
    }
    else
    {
        wasm_builder::call(*cur_expr, func_call.function_name.c_str());
    }
}

void lir_based_emitter::emit(const lir::if_expression& if_expr)
{
    wasm_expr* prev_expr = cur_expr;
    wasm_control_block* prev_control_block = cur_control_block;
    emit(*if_expr.condition);
    auto& if_block = wasm_builder::if_block(
        *cur_expr,
        cur_control_block,
        lowering->lower_return_type(if_expr.assigned_type)
    );

    cur_control_block = &if_block;
    cur_expr = &if_block.then_block;
    for (const auto& expression : if_expr.then_code)
    {
        emit(*expression);
    }
    if (!if_expr.then_code.empty())
    {
        cur_expr = &if_block.else_block;
        for (const auto& expression : if_expr.else_code)
        {
            emit(*expression);
        }
    }
    cur_control_block = prev_control_block;
    cur_expr = prev_expr;
}

void lir_based_emitter::emit(const lir::load_expression& load)
{
    auto source_var = parse_context.symbol_at(load.source.base.symbol_ref);
    storage_loc sl = transform_symbol(parse_context, source_var);
    for (const auto& elem : load.source.projection)
    {
        sl = transform_place(parse_context, sl, elem, *cur_expr);
    }
    transform_load(parse_context, sl, *cur_expr);
}

void lir_based_emitter::emit(const lir::allocate_record_expression& expr)
{
    auto type_spec = parse_context.record_behind(expr.type);
    CAPY_ASSERT(type_spec.has_value(), "Unexpected failure in extracting pointer type for record");
    type_id r_type = type_spec.value();

    size_t record_size = lir_type_size(parse_context, type_spec.value());

    wasm_builder::const_val(*cur_expr, wasm_type::i32, 0);
    wasm_builder::const_val(*cur_expr, wasm_type::i32, 0);
    wasm_builder::const_val(*cur_expr, wasm_type::i32, 4);
    wasm_builder::const_val(*cur_expr, wasm_type::i32, lir_type_size(parse_context, r_type));
    wasm_builder::call(*cur_expr, "cabi_realloc");
}

void lir_based_emitter::emit(const lir::string_literal& literal)
{
    // TODO: here we just lower to pointer + length blindly, but we might have to
    // move this part into the lowering strategies
    uint32_t ptr = parse_context.string_literals[literal.table_index].start_address;
    wasm_builder::const_val(*cur_expr, wasm_type::i32, ptr);
    wasm_builder::const_val(*cur_expr, wasm_type::i32, literal.size);
}

void lir_based_emitter::emit(const lir::bool_literal& bool_const)
{
    wasm_builder::const_val(*cur_expr, wasm_type::i32, bool_const.value ? 1 : 0);
}

void lir_based_emitter::emit(const lir::char_literal& literal)
{
    wasm_builder::const_val(*cur_expr, wasm_type::i32, literal.ch);
}

void lir_based_emitter::emit(const lir::number& number)
{
    // the constant gets the WASM type of the literal (e.g. i64 for u64/s64)
    wasm_builder::const_val(*cur_expr, value_type_of(number.assigned_type), number.value);
}

wasm_type lir_based_emitter::value_type_of(type_id type) const
{
    type_id resolved = parse_context.resolved_type(type);
    if (!parse_context.is_resolved(resolved))
    {
        // Unsuffixed literals without any typed counterpart (e.g. '5 + 3')
        // aren't resolved by the type inference; they default to i32.
        return wasm_type::i32;
    }
    return lowering->lower_return_type(resolved);
}

uint32_t lir_based_emitter::allocate_data(const std::string& data)
{
    return cur_data->allocate_data(data);
}
