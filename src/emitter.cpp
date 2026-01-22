#include "emitter.hpp"
#include "ir.hpp"
#include "semantics.hpp"
#include <algorithm>
#include <cassert>
#include <iomanip>
#include <ranges>
#include <sstream>

std::string create_wasm_name(const std::string capy_name)
{
    return "$" + capy_name;
}

wasm_type from_type_kind(context& ctx, type_id idx)
{
    const auto& type_entry = ctx.types[idx];

    return std::visit([&](const auto &t) -> auto {
        using T = std::decay_t<decltype(t)>;

        if constexpr (std::is_same_v<T, type_kind>)
        {
            const auto& type_spec = t;

            return std::visit([&](const auto& k) -> wasm_type {
                using K = std::decay_t<decltype(k)>;

                if constexpr (std::is_same_v<K, primitive_type>) {
                    switch (k) {
                        case primitive_type::Void:
                            return wasm_type::none;
                        case primitive_type::Char:
                        case primitive_type::S8:
                            return wasm_type::i8;
                        case primitive_type::S16:
                            return wasm_type::i16;
                        case primitive_type::S32:
                            return wasm_type::i32;
                        case primitive_type::U8:
                            return wasm_type::u8;
                        case primitive_type::U16:
                            return wasm_type::u16;
                        case primitive_type::U32:
                            return wasm_type::u32;
                        case primitive_type::Boolean:
                            return wasm_type::i32;
                        case primitive_type::String:
                            return wasm_type::i32;
                        default:
                            printf("Unknown primitive type in from_type_kind\n");
                            return wasm_type::none;
                    }
                } else if constexpr (std::is_same_v<K, pointer_type>) {
                    return wasm_type::i32;
                } else if constexpr (std::is_same_v<K, record_type>) {
                    // records are stored as pointers
                    return wasm_type::i32;
                } else if constexpr (std::is_same_v<K, function_type>) {
                    printf("Requested function type in conversion\n");
                    return wasm_type::none;
                } else {
                    printf("Unknown type kind in from_type_kind\n");
                    return wasm_type::none;
                }
            }, type_spec);
        }
        else if constexpr (std::is_same_v<T, type_var>)
        {
            if (t.parent.has_value())
            {
                return from_type_kind(ctx, t.parent.value());
            }
            else
            {
                assert(false && "Encountered unresolved type variable when getting WASM type");
                return wasm_type::none;
            }
        }
    }, type_entry);
}

size_t type_size(context& ctx, type_id idx)
{
    const auto& type_entry = ctx.types[idx];

    return std::visit([&](const auto &t) -> size_t {
        using T = std::decay_t<decltype(t)>;

        if constexpr (std::is_same_v<T, type_kind>)
        {
            auto type_spec = t;

            return std::visit([&](const auto &k) -> size_t {
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
                        case primitive_type::String:
                            return 8; // ptr and size fields
                    }
                } else if constexpr (std::is_same_v<K, pointer_type>) {
                    return 4;
                } else if constexpr (std::is_same_v<K, record_type>) {
                    size_t size = 0;
                    for (const auto& field : k.fields) {
                        size += type_size(ctx, field.second);
                    }
                    return size;
                } else {
                    return 0;
                }
            }, type_spec);
        }
        else if constexpr (std::is_same_v<T, type_var>)
        {
            if (t.parent.has_value())
            {
                return type_size(ctx, t.parent.value());
            }
            else
            {
                assert(false && "Encountered unresolved type variable in when getting type size");
                return 0;
            }
        }
    }, type_entry);

}

std::optional<size_t> record_field_offset(context& ctx, type_id idx, const std::string& field_name)
{
    if (!ctx.is_record_type(idx))
    {
        return std::nullopt;
    }

    const auto& type_spec = ctx.types[idx];
    const record_type& r = std::get<record_type>(std::get<type_kind>(type_spec));

    size_t offset = 0;
    for (const auto& field_definition : r.fields)
    {
        if (field_definition.first == field_name)
        {
            return offset;
        }
        offset += type_size(ctx, field_definition.second);
    }

    // we didn't find a field with the given name yet, so it doesn't exist in this definition
    return std::nullopt;
}

emitter::emitter(context& ctx)
: parse_context(ctx)
{
    cur_data = new wasm_data_section(100);
    allocate_data(std::string("\x42\x00\x00\x00\x10\x00\x00\x00\x10\x20\x30\x40Test", 16));
}

emitter::~emitter()
{
    delete cur_data;
}

wasm_module emitter::generate(node_module &module_def)
{
    wasm_module ir_module;
    cur_mod = &ir_module;

    current_module = &module_def;

    auto& memory = ir_module.create_memory(2);
    ir_module.export_as("memory", memory);

    for (auto& literal: module_def.string_literals)
    {
        literal.start_address = allocate_data(literal.literal);
    }

    for (const auto &import_def : module_def.imports)
    {
        emit(*import_def);
    }

    for (const auto &global_def : module_def.globals)
    {
        emit(*global_def);
    }

    for (const auto &func_def : module_def.functions)
    {
        emit(*func_def);
    }

    cur_mod->append_data_section(*cur_data);

    current_module = nullptr;
    cur_mod = nullptr;

    return ir_module;
}

void emitter::emit(ast_node &node)
{
    std::visit([this](auto &n)
    {
        this->emit(n);
    }, node.value);
}

void emitter::emit(const node_import_definition &import_def)
{
    arguments_type args;
    auto function_name = import_def.function_head->name;

    auto function_type_entry = parse_context.types[import_def.function_head->signature.function_type];
    auto func_type = std::get<function_type>(std::get<type_kind>(function_type_entry));

    const auto& parameter_names = import_def.function_head->signature.parameters;
    for (auto [param_name, param_typ] : std::views::zip(parameter_names,
                                                    func_type.parameter_types))
    {
        args.push_back({param_name, from_type_kind(parse_context, param_typ)});
    }
    // TODO: we should actually use the alias name here if it is defined
    auto& import_func = cur_mod->create_function(import_def.function_head->name.c_str(),
        from_type_kind(parse_context,
                       parse_context.function_return_type(
                            import_def.function_head->signature.function_type).value()),
        args
    );
    import_func.import_from(import_def.ns_name.c_str(), import_def.function_head->name.c_str());

    emit(*import_def.function_head);
}

void emitter::emit(const node_global& global_def)
{
    // if (!global_def.init_expression)
    // {
    //     // TODO: maybe all globals should be initialised
    //     // The variable is not initialised, so we don't need to evaluate any
    //     // initialiser expressions
    //     return;
    // }

    // emit(*global_def.init_expression);
    // if (t_t::is_of<t_t::string>(global_def.symbol_ref.get().symbol_type))
    // {
    //     cur_block->global_set((global_def.symbol_ref.get().name+"_ptr").c_str());
    //     cur_block->global_set((global_def.symbol_ref.get().name+"_size").c_str());
    // }
    // else
    // {
    //     cur_block->global_set(global_def.symbol_ref.get().name.c_str());
    // }
    auto mutability = global_def.symbol_ref.get().mutab ?
                            wasm_module::access_type::mut :
                            wasm_module::access_type::immut;
    cur_mod->create_global(global_def.symbol_ref.get().name.c_str(),
                           wasm_type::i32,
                           mutability,
                           global_def.init_value);
}

void emitter::emit(const node_function_head &function_head)
{
    emit_function_signature(function_head.name, function_head.signature);
}

void emitter::emit(const node_function_definition &func_def)
{
    arguments_type args;
    auto function_name = func_def.function_head->name;

    auto function_type_entry = parse_context.types[func_def.function_head->signature.function_type];
    auto func_type = std::get<function_type>(std::get<type_kind>(function_type_entry));

    const auto& parameter_names = func_def.function_head->signature.parameters;
    for (auto [param_name, param_typ] : std::views::zip(parameter_names,
                                                    func_type.parameter_types))
    {
        args.push_back({param_name, from_type_kind(parse_context, param_typ)});
    }

    auto &func = cur_mod->create_function(function_name.c_str(),
        from_type_kind(parse_context,
            parse_context.function_return_type(
                func_def.function_head->signature.function_type).value()),
        args);
    if (func_def.has_attribute("export"))
    {
        cur_mod->export_as(function_name.c_str(), func);
    }
    cur_block = &func.body();

    emit(*func_def.function_head);

    for (auto& [identifier, symbol] : func_def.function_scope->symbol_table)
    {
        if (symbol.kind == symbol_kind::local_var)
        {
            if (parse_context.is_primitive_type(symbol.symbol_type, primitive_type::String))
            {
                func.allocate_local((identifier+"_ptr").c_str(), wasm_type::i32);
                func.allocate_local((identifier+"_size").c_str(), wasm_type::i32);
            }
            else
            {
                auto t = from_type_kind(parse_context, symbol.symbol_type);
                func.allocate_local(identifier.c_str(), t);
            }
        }
    }
    func.allocate_local("_rec_ptr", wasm_type::i32);

    for (const auto &expression : func_def.code)
    {
        emit(*expression);
    }
}

void emitter::emit(const node_record_definition& record_def)
{
}

void emitter::emit(const node_if_expression& if_expr)
{
    emit(*if_expr.condition);
    auto [then_block, else_block] = cur_block->if_block(from_type_kind(parse_context, if_expr.assigned_type));

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

void emitter::emit(const node_while_expression& while_expr)
{
    wasm_block* prev_block = cur_block;

    wasm_block& while_exit = cur_block->block(wasm_type::none);
    wasm_block& while_block = while_exit.loop(wasm_type::none);
    cur_block = &while_block;

    emit(*while_expr.condition);

    cur_block->eqz(wasm_type::i32);
    cur_block->br_if(while_exit.label());

    for (const auto& expression : while_expr.while_code)
    {
        emit(*expression);
    }
    cur_block->br(while_block.label());

    cur_block = prev_block;
}

void emitter::emit(const node_function_call &func_call)
{
    for (const auto &param : func_call.parameter)
    {
        emit(*param);
    }
    cur_block->call(func_call.function_name.c_str());
}

void emitter::emit(const node_let_expression& let_expression)
{
    if (!let_expression.init_expression)
    {
        // The variable is not initialised, so we don't need to evaluate any
        // initialiser expressions
        return;
    }

    emit(*let_expression.init_expression);
    if (parse_context.is_primitive_type(let_expression.symbol_ref.get().symbol_type, primitive_type::String))
    {
        cur_block->local_set((let_expression.symbol_ref.get().name+"_ptr").c_str());
        cur_block->local_set((let_expression.symbol_ref.get().name+"_size").c_str());
    }
    else
    {
        cur_block->local_set(let_expression.symbol_ref.get().name.c_str());
    }
}

void emitter::emit(const node_record_initialisation& record_init)
{
    size_t record_size = type_size(parse_context, record_init.type_spec);
    cur_block->const_val(wasm_type::i32, 0);
    cur_block->const_val(wasm_type::i32, 0);
    cur_block->const_val(wasm_type::i32, 4);
    cur_block->const_val(wasm_type::i32, record_size);
    cur_block->call("cabi_realloc");
    cur_block->local_set("_rec_ptr");

    // TODO: semantic check should make sure, that the type_spec really is a record
    const auto& type_spec = parse_context.types[record_init.type_spec];
    auto& rec_type = std::get<record_type>(std::get<type_kind>(type_spec));

    size_t offset = 0;
    // TODO: make the inverse check if any field init tries to init a field that isn't
    // in the definition
    for (const auto& field : rec_type.fields)
    {
        // get the address for the field to initialise
        cur_block->local_get("_rec_ptr");

        // We go through the record definition to keep the order of the fields as given
        // in the definition; we assume, that field initialisations can be out of the
        // definition order, so we need to match initialisations with the field definitions
        for (const auto& field_init : record_init.initialisations)
        {
            if (field_init.field_name == field.first)
            {
                // generate code to calculate the initialisation value of this field
                emit(*field_init.init_expression);
                break;
            }
        }
        // TODO: Make sure that every field of the type definition is initialised

        // save the value in the record at the current offset
        cur_block->store(wasm_type::i32, offset);

        offset += type_size(parse_context, field.second);
    }

    // after record initialisation, leave the pointer of the newly created record
    // on the stack
    cur_block->local_get("_rec_ptr");
}

void emitter::emit(const node_field_deref& field_deref)
{
    if (parse_context.is_primitive_type(field_deref.object_type, primitive_type::String))
    {
        // TODO: for the moment we can only handle variables of types strings
        // but not strings that are fields of records
        auto var_name = std::get<node_var_reference>(field_deref.object->value).name;
        cur_block->local_get((var_name + "_"+field_deref.fieldname).c_str());
    }
    else
    {
        emit(*field_deref.object);
        auto maybe_size = record_field_offset(
            parse_context,
            field_deref.object_type,
            field_deref.fieldname);
        assert(maybe_size.has_value() && "A record field should have its offset calculated by this point");

        cur_block->load(wasm_type::i32, maybe_size.value());
    }
}

void emitter::emit(const node_cast_expression &root)
{
    emit(*root.expression);
    if ((parse_context.is_primitive_type(root.cast_type, primitive_type::Void)) &&
        (!parse_context.is_primitive_type(assigned_node_type(*root.expression, parse_context),
                primitive_type::Void)))
    {
        cur_block->drop();
    }
}

void emitter::emit(const node_expression &root)
{
    emit(*root.left);
    emit(*root.right);
    switch (root.operation)
    {
    case op_minus:
        cur_block->sub(wasm_type::i32);
        break;
    case op_plus:
        cur_block->add(wasm_type::i32);
        break;
    case op_multiply:
        cur_block->mul(wasm_type::i32);
        break;
    case op_division:
        cur_block->div(wasm_type::i32);
        break;
    case op_modulus:
        cur_block->mod(wasm_type::i32);
        break;
    case op_equals:
        cur_block->eq(wasm_type::i32);
        break;
    case op_notequals:
        cur_block->ne(wasm_type::i32);
        break;
    case op_assignment:
        if (std::holds_alternative<node_pointer_deref>(root.left->value))
        {
            if (parse_context.is_primitive_type(assigned_node_type(*root.left, parse_context), primitive_type::U8))
            {
                cur_block->store(wasm_type::i8);
                break;
            }
            else
            {
                cur_block->store(wasm_type::i32);
                break;
            }
        }
        else
        {
            auto symbol = std::get<node_var_reference>(root.left->value).symbol_ref;
            if (symbol.get().kind == symbol_kind::global_var)
            {
                cur_block->global_set(symbol.get().name.c_str());                
            }
            else
            {
                cur_block->local_set(symbol.get().name.c_str());
            }
            break;
        }
    case op_conversion:
        assert(false && "Conversions should not appear as simple expressions");
        break;
    }
}

void emitter::emit(const node_string_literal& literal)
{
    uint32_t ptr = current_module->string_literals[literal.table_index].start_address;
    cur_block->const_val(wasm_type::i32, literal.size);
    cur_block->const_val(wasm_type::i32, ptr);
}

void emitter::emit(const node_char_literal& literal)
{
    cur_block->const_val(wasm_type::i32, literal.ch);
}

void emitter::emit(const node_bool_const& bool_const)
{
    cur_block->const_val(wasm_type::i32, bool_const.value ? 1 : 0);
}

void emitter::emit(const node_number &number)
{
    cur_block->const_val(wasm_type::i32, number.number);
}

void emitter::emit(const node_var_reference &variable)
{
    // When the variable is used in an LHS context, then we need to
    // handle the storing of the value differently
    if (variable.context == assign_context::rhs)
    {
        auto symbol = variable.symbol_ref;
        if (symbol.get().kind == symbol_kind::global_var)
        {
            cur_block->global_get(symbol.get().name.c_str());
        }
        else
        {
            cur_block->local_get(symbol.get().name.c_str());
        }    
    }
}

void emitter::emit(const node_pointer_deref& ptr_deref)
{
    emit(*ptr_deref.pointer_expression);
    if (ptr_deref.context == assign_context::rhs)
    {
        if (parse_context.is_primitive_type(ptr_deref.assigned_type,
                                            primitive_type::U8))
        {
            cur_block->load(wasm_type::u8);
        }
        else
        {
            cur_block->load(wasm_type::i32);
        }
    }
    else
    {
        // in LHS context we have to get address, which in our
        // case means to get the index of the variable
        cur_block->local_get(std::get<node_var_reference>(ptr_deref.pointer_expression->value).symbol_ref.get().name.c_str());
    }
}

void emitter::emit_function_signature(const std::string &function_name, const function_signature &signature)
{
}

uint32_t emitter::allocate_data(const std::string& data)
{
    return cur_data->allocate_data(data);
}
