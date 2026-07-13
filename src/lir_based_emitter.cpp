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
                            // case primitive_type::String:
                            //     return 8; // ptr and size fields
                        }
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

storage_loc transform_place(const context& parse_context, const storage_loc& storage, const lir::place_elem& place, wasm_block& code_block)
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
                            code_block.local_get(arg.name.c_str());
                            // TODO: These .value() are actually ugly and we should make
                            // sure that they are really resolved
                            deref_type = parse_context.derefed_type(arg.type).value();
                        },
                        [&](const sl_local_var& local)
                        {
                            code_block.local_get(local.name.c_str());
                            deref_type = parse_context.derefed_type(local.type).value();
                        },
                        [&](const sl_global_var& global)
                        {
                            code_block.global_get(global.name.c_str());
                            deref_type = parse_context.derefed_type(global.type).value();
                        },
                        [&](const sl_ptr& ptr)
                        {
                            // The type for loading is always fixed here, because we know
                            // that it is a pointer; TODO: maybe introduce a ptr-type in
                            // the WASM IR.
                            // The offset is taken from the given sl_ptr because we need
                            // to include this in our load here.
                            code_block.load(wasm_type::i32, ptr.offset);
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

void transform_load(const context& parse_context, const storage_loc& storage, wasm_block& code_block)
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
                        code_block.local_get((arg.name + "_" + std::to_string(field_index)).c_str());
                        field_index++;
                    }
                }
                else
                {
                    code_block.local_get(arg.name.c_str());
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
                        code_block.local_get((local.name + "_" + std::to_string(field_index)).c_str());
                        field_index++;
                    }
                }
                else
                {
                    code_block.local_get(local.name.c_str());
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
                        code_block.global_get((global.name + "_" + std::to_string(field_index)).c_str());
                        field_index++;
                    }
                }
                else
                {
                    code_block.global_get(global.name.c_str());
                }
            },
            [&](const sl_ptr& ptr)
            {
                if (parse_context.is_primitive_type(ptr.type, primitive_type::U8))
                {
                    // TODO: we should simplify this by using a mapping from
                    // primitive types to wasm_types
                    code_block.load(wasm_type::u8, ptr.offset);
                }
                else
                {
                    code_block.load(wasm_type::i32, ptr.offset);
                }
            }
        }
    );
}

lir_based_emitter::lir_based_emitter(context& ctx)
: parse_context(ctx)
, lowering(new lower_cabi(ctx))
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

    auto& memory = ir_module.create_memory(2);
    ir_module.export_as("memory", memory);

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
    auto mutability = global_symbol.mutab ? wasm_module::access_type::mut : wasm_module::access_type::immut;
    cur_mod->create_global(global_symbol.name.c_str(), wasm_type::i32, mutability, global_def.init_value);
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
    cur_block = &func.body();

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
    wasm_block* prev_block = cur_block;

    wasm_block& while_exit = cur_block->block(wasm_type::none);
    wasm_block& while_block = while_exit.loop(wasm_type::none);
    cur_block = &while_block;

    emit(*while_stmt.condition);

    cur_block->eqz(wasm_type::i32);
    cur_block->br_if(while_exit.label());

    for (const auto& expression : while_stmt.while_code)
    {
        emit(*expression);
    }
    cur_block->br(while_block.label());

    cur_block = prev_block;
}

void lir_based_emitter::emit(const lir::store_statement& store)
{
    const auto& target_sym = parse_context.symbol_at(store.target.base.symbol_ref);

    // Turn projections into dereferencing operations and index/offset calculations

    // TODO: generate any accessor elements/operations for this store and create
    // an object that could be used to determine the type of operation
    if (!store.target.projection.empty())
    {
        // TODO: check that the last projection is actual a pointer deref
        if (target_sym.kind == symbol_kind::global_var)
        {
            cur_block->global_get(target_sym.name.c_str());
        }
        else if (target_sym.kind == symbol_kind::local_var)
        {
            cur_block->local_get(target_sym.name.c_str());
        }
    }

    emit(*store.value);

    if (store.target.projection.empty())
    {
        if (target_sym.kind == symbol_kind::global_var)
        {
            cur_block->global_set(target_sym.name.c_str());
        }
        else if (target_sym.kind == symbol_kind::local_var)
        {
            cur_block->local_set(target_sym.name.c_str());
        }
    }
    else
    {
        // TODO: check that it actually is pointer deref
        auto base_type = parse_context.find_base_type(target_sym.symbol_type);
        CAPY_ASSERT(base_type.has_value(), "LIR emitter: Expecting there to be a pointer type");
        if (parse_context.is_primitive_type(base_type.value(), primitive_type::U8))
        {
            cur_block->store(wasm_type::i8);
        }
        else
        {
            cur_block->store(wasm_type::i32);
        }
    }
}

void lir_based_emitter::emit(const lir::store_string_statement& store)
{
    const auto& target_sym = parse_context.symbol_at(store.target.base.symbol_ref);

    uint32_t ptr = parse_context.string_literals[store.string_table_index].start_address;
    cur_block->const_val(wasm_type::i32, ptr);
    cur_block->const_val(wasm_type::i32, store.string_size);

    if (target_sym.kind == symbol_kind::global_var)
    {
        cur_block->global_set((target_sym.name + "_1").c_str());
        cur_block->global_set((target_sym.name + "_0").c_str());
    }
    else if (target_sym.kind == symbol_kind::local_var)
    {
        cur_block->local_set((target_sym.name + "_1").c_str());
        cur_block->local_set((target_sym.name + "_0").c_str());
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
        cur_block->drop();
    }
}

void lir_based_emitter::emit(const lir::break_statement& root)
{
    // find the nearest loop block (currently just assuming that this will be
    // a proper loop like a 'while') and use that block as branch target
    wasm_block* b = cur_block;
    while (b->block_type != wasm_block::t_loop && b->enclosing_block != nullptr)
    {
        b = b->enclosing_block;
    }
    if (b->block_type == wasm_block::t_loop)
    {
        // TODO: This feels a little hacky; we assume that a loop is always
        // enclosed in a plain 'block' because it is constructed from a while
        // loop. This however may not always be the case in the future and we
        // should instead make sure, we really target the right block for
        // branch instead of heuristically guessing it.
        cur_block->br(b->enclosing_block->label());
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
        cur_block->ret();
    }
}

void lir_based_emitter::emit(const lir::binary_expression& root)
{
    emit(*root.left);
    emit(*root.right);
    switch (root.op)
    {
        case lir::binary_op_t::Minus:
            cur_block->sub(wasm_type::i32);
            break;
        case lir::binary_op_t::Plus:
            cur_block->add(wasm_type::i32);
            break;
        case lir::binary_op_t::Multiply:
            cur_block->mul(wasm_type::i32);
            break;
        case lir::binary_op_t::Division:
            cur_block->div(wasm_type::i32);
            break;
        case lir::binary_op_t::Modulus:
            cur_block->mod(wasm_type::i32);
            break;
        case lir::binary_op_t::AndOp:
            cur_block->and_instr(wasm_type::i32);
            break;
        case lir::binary_op_t::OrOp:
            cur_block->or_instr(wasm_type::i32);
            break;
        case lir::binary_op_t::Shl:
            cur_block->shl(wasm_type::i32);
            break;
        case lir::binary_op_t::Shr:
            cur_block->shr(wasm_type::i32);
            break;
        case lir::binary_op_t::Equals:
            cur_block->eq(wasm_type::i32);
            break;
        case lir::binary_op_t::NotEquals:
            cur_block->ne(wasm_type::i32);
            break;
        case lir::binary_op_t::LessThan:
            cur_block->lt(wasm_type::i32);
            break;
        case lir::binary_op_t::LessThanEqual:
            cur_block->lte(wasm_type::i32);
            break;
        case lir::binary_op_t::GreaterThan:
            cur_block->gt(wasm_type::i32);
            break;
        case lir::binary_op_t::GreaterThanEqual:
            cur_block->gte(wasm_type::i32);
            break;
    }
}

void lir_based_emitter::emit(const lir::unary_expression& root)
{
    cur_block->const_val(wasm_type::i32, 0);
    emit(*root.expr);
    cur_block->sub(wasm_type::i32);
}

void lir_based_emitter::emit(const lir::cast_expression& root)
{
    emit(*root.expression);
    if ((parse_context.is_primitive_type(root.cast_type, primitive_type::Void)) &&
        (!parse_context.is_primitive_type(lir_assigned_node_type(*root.expression, parse_context), primitive_type::Void)))
    {
        cur_block->drop();
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
            cur_block->memory_size();
        }
        else if (func_symbol.name == "memory_grow")
        {
            cur_block->memory_grow();
        }
        else if (func_symbol.name == "unreachable")
        {
            cur_block->unreachable();
        }
        else
        {
            CAPY_FAIL("Unknown intrinsic");
        }
    }
    else
    {
        cur_block->call(func_call.function_name.c_str());
    }
}

void lir_based_emitter::emit(const lir::if_expression& if_expr)
{
    emit(*if_expr.condition);
    auto [then_block, else_block] = cur_block->if_block(lowering->lower_return_type(if_expr.assigned_type));

    auto* prev_block = cur_block;
    cur_block = &then_block;
    for (const auto& expression : if_expr.then_code)
    {
        emit(*expression);
    }
    if (!if_expr.then_code.empty())
    {
        cur_block = &else_block;
        for (const auto& expression : if_expr.else_code)
        {
            emit(*expression);
        }
    }
    cur_block = prev_block;
}

void lir_based_emitter::emit(const lir::load_expression& load)
{
    auto source_var = parse_context.symbol_at(load.source.base.symbol_ref);
    storage_loc sl = transform_symbol(parse_context, source_var);
    for (const auto& elem : load.source.projection)
    {
        sl = transform_place(parse_context, sl, elem, *cur_block);
    }
    transform_load(parse_context, sl, *cur_block);
}

void lir_based_emitter::emit(const lir::allocate_record_expression& expr)
{
    auto type_spec = parse_context.record_behind(expr.type);
    CAPY_ASSERT(type_spec.has_value(), "Unexpected failure in extracting pointer type for record");
    type_id r_type = type_spec.value();

    size_t record_size = lir_type_size(parse_context, type_spec.value());

    cur_block->const_val(wasm_type::i32, 0);
    cur_block->const_val(wasm_type::i32, 0);
    cur_block->const_val(wasm_type::i32, 4);
    cur_block->const_val(wasm_type::i32, 0);
    cur_block->call("cabi_realloc");
}

void lir_based_emitter::emit(const lir::string_literal& literal)
{
    // TODO: here we just lower to pointer + length blindly, but we might have to
    // move this part into the lowering strategies
    uint32_t ptr = parse_context.string_literals[literal.table_index].start_address;
    cur_block->const_val(wasm_type::i32, ptr);
    cur_block->const_val(wasm_type::i32, literal.size);
}

void lir_based_emitter::emit(const lir::bool_literal& bool_const)
{
    cur_block->const_val(wasm_type::i32, bool_const.value ? 1 : 0);
}

void lir_based_emitter::emit(const lir::char_literal& literal)
{
    cur_block->const_val(wasm_type::i32, literal.ch);
}

void lir_based_emitter::emit(const lir::number& number)
{
    cur_block->const_val(wasm_type::i32, number.value);
}

uint32_t lir_based_emitter::allocate_data(const std::string& data)
{
    return cur_data->allocate_data(data);
}
