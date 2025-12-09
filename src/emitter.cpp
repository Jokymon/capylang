#include "emitter.hpp"
#include "ir.hpp"
#include "semantics.hpp"
#include "wat_generator.hpp"
#include <algorithm>
#include <cassert>
#include <iomanip>
#include <sstream>

std::string create_wasm_name(const std::string capy_name)
{
    return "$" + capy_name;
}

wasm_type from_type_kind(const type_kind& type_spec)
{
    return std::visit([&](const auto &t) -> auto {
        using T = std::decay_t<decltype(t)>;

        if constexpr (std::is_same_v<T, t_t::s32>) {
            return wasm_type::i32;
        } else if constexpr (std::is_same_v<T, t_t::s16>) {
            return wasm_type::i16;
        } else if constexpr (std::is_same_v<T, t_t::s8>) {
            return wasm_type::i8;
        } else if constexpr (std::is_same_v<T, t_t::u8>) {
            return wasm_type::u8;
        } else if constexpr (std::is_same_v<T, t_t::u16>) {
            return wasm_type::u16;
        } else if constexpr (std::is_same_v<T, t_t::u32>) {
            return wasm_type::u32;
        } else if constexpr (std::is_same_v<T, t_t::pointer>) {
            return wasm_type::i32;
        } else {
            return wasm_type::none;
        }
    }, type_spec);
}

size_t type_size(const type_kind& type_spec)
{
    return std::visit([&](const auto &t) -> size_t {
        using T = std::decay_t<decltype(t)>;

        if constexpr (std::is_same_v<T, t_t::s32>) {
            return 4;
        } else if constexpr (std::is_same_v<T, t_t::s16>) {
            return 2;
        } else if constexpr (std::is_same_v<T, t_t::s8>) {
            return 1;
        } else if constexpr (std::is_same_v<T, t_t::u8>) {
            return 1;
        } else if constexpr (std::is_same_v<T, t_t::u16>) {
            return 2;
        } else if constexpr (std::is_same_v<T, t_t::u32>) {
            return 4;
        } else if constexpr (std::is_same_v<T, t_t::pointer>) {
            return 4;
        } else if constexpr (std::is_same_v<T, t_t::record>) {
            size_t size = 0;
            for (const auto& field : t.fields) {
                size += type_size(*field.type_spec);
            }
            return size;
        } else {
            return 0;
        }
    }, type_spec);
}

std::optional<size_t> record_field_offset(const type_kind& record, const std::string& field_name)
{
    if (!std::holds_alternative<t_t::record>(record))
    {
        return std::nullopt;
    }

    const t_t::record& r = std::get<t_t::record>(record);

    size_t offset = 0;
    for (const auto& field_definition : r.fields)
    {
        if (field_definition.name == field_name)
        {
            return offset;
        }
        offset += type_size(*field_definition.type_spec);
    }

    // we didn't find a field with the given name yet, so it doesn't exist in this definition
    return std::nullopt;
}

emitter::emitter(std::ostream &output) : output_(output) //, data_buffer(""), data_offset(100), id_gen(0)
{
    cur_data = new wasm_data_section(100);
    allocate_data(std::string("\x42\x00\x00\x00\x10\x00\x00\x00\x10\x20\x30\x40Test", 16));
}

emitter::~emitter()
{
    delete cur_data;
}

void emitter::generate(node_module &module_def)
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

    wat_generator generator;
    generator.generate(ir_module, output_);

    current_module = nullptr;
    cur_mod = nullptr;
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
    for (const auto &param : import_def.function_head->signature.parameters)
    {
        args.push_back({param.name, from_type_kind(param.type_spec)});
    }
    // TODO: we should actually use the alias name here if it is defined
    auto& import_func = cur_mod->create_function(import_def.function_head->name.c_str(),
        from_type_kind(import_def.function_head->signature.return_type),
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
    for (const auto &param : func_def.function_head->signature.parameters)
    {
        args.push_back({param.name, from_type_kind(param.type_spec)});
    }
    auto &func = cur_mod->create_function(function_name.c_str(), from_type_kind(func_def.function_head->signature.return_type), args);
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
            if (t_t::is_of<t_t::string>(symbol.symbol_type))
            {
                func.allocate_local((identifier+"_ptr").c_str(), wasm_type::i32);
                func.allocate_local((identifier+"_size").c_str(), wasm_type::i32);
            }
            else
            {
                func.allocate_local(identifier.c_str(), wasm_type::i32);
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
    auto [then_block, else_block] = cur_block->if_block(from_type_kind(if_expr.assigned_type));

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
    if (t_t::is_of<t_t::string>(let_expression.symbol_ref.get().symbol_type))
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
    size_t record_size = type_size(record_init.type_spec);
    cur_block->const_val(wasm_type::i32, 0);
    cur_block->const_val(wasm_type::i32, 0);
    cur_block->const_val(wasm_type::i32, 4);
    cur_block->const_val(wasm_type::i32, record_size);
    cur_block->call("cabi_realloc");
    cur_block->local_set("_rec_ptr");

    // TODO: semantic check should make sure, that the type_spec really is a record
    auto& record_type = std::get<t_t::record>(record_init.type_spec);

    size_t offset = 0;
    // TODO: make the inverse check if any field init tries to init a field that isn't
    // in the definition
    for (const auto& field : record_type.fields)
    {
        // get the address for the field to initialise
        cur_block->local_get("_rec_ptr");

        // We go through the record definition to keep the order of the fields as given
        // in the definition; we assume, that field initialisations can be out of the
        // definition order, so we need to match initialisations with the field definitions
        for (const auto& field_init : record_init.initialisations)
        {
            if (field_init.field_name == field.name)
            {
                // generate code to calculate the initialisation value of this field
                emit(*field_init.init_expression);
                break;
            }
        }
        // TODO: Make sure that every field of the type definition is initialised

        // save the value in the record at the current offset
        cur_block->store(wasm_type::i32, offset);

        offset += type_size(*field.type_spec);
    }

    // after record initialisation, leave the pointer of the newly created record
    // on the stack
    cur_block->local_get("_rec_ptr");
}

void emitter::emit(const node_field_deref& field_deref)
{
    if (t_t::is_of<t_t::string>(field_deref.object_type))
    {
        // TODO: for the moment we can only handle variables of types strings
        // but not strings that are fields of records
        auto var_name = std::get<node_var_reference>(field_deref.object->value).name;
        cur_block->local_get((var_name + "_"+field_deref.fieldname).c_str());
    }
    else
    {
        emit(*field_deref.object);
        auto maybe_size = record_field_offset(field_deref.object_type, field_deref.fieldname);
        assert(maybe_size.has_value() && "A record field should have its offset calculated by this point");

        cur_block->load(wasm_type::i32, maybe_size.value());
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
            if (t_t::is_of<t_t::u8>(assigned_node_type(*root.left)))
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
        if ((t_t::is_of<t_t::void_type>(assigned_node_type(*root.right))) &&
            (!t_t::is_of<t_t::void_type>(assigned_node_type(*root.left))))
        {
            cur_block->drop();
        }
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

void emitter::emit(const node_type_spec& spec)
{
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
        if (t_t::is_of<t_t::u8>(ptr_deref.assigned_type))
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
