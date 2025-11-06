#include "emitter.hpp"
#include "semantics.hpp"
#include <algorithm>
#include <cassert>
#include <iomanip>
#include <sstream>

std::string create_wasm_name(const std::string capy_name)
{
    return "$" + capy_name;
}

std::string type_mapping(const type_kind& type_spec)
{
    return std::visit([&](const auto &t) -> std::string {
        using T = std::decay_t<decltype(t)>;

        if constexpr (std::is_same_v<T, t_t::s32>) {
            return "i32";
        } else if constexpr (std::is_same_v<T, t_t::u8>) {
            return "i32";
        } else if constexpr (std::is_same_v<T, t_t::u32>) {
            return "i32";
        } else if constexpr (std::is_same_v<T, t_t::pointer>) {
            return "i32";
        } else {
            return "";
        }
    }, type_spec);
}

size_t type_size(const type_kind& type_spec)
{
    return std::visit([&](const auto &t) -> size_t {
        using T = std::decay_t<decltype(t)>;

        if constexpr (std::is_same_v<T, t_t::s32>) {
            return 4;
        } else if constexpr (std::is_same_v<T, t_t::u8>) {
            return 1;
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

emitter::emitter(std::ostream &output) : output_(output), data_buffer(""), data_offset(100)
{
    allocate_data(std::string("\x42\x00\x00\x00\x10\x00\x00\x00\x10\x20\x30\x40Test", 16));
}

void emitter::generate(ast_node &node)
{
    this->emit(node);
}

void emitter::emit(ast_node &node)
{
    std::visit([this](auto &n)
    {
        this->emit(n);
    }, node.value);
}

void emitter::emit(node_module &module_def)
{
    current_module = &module_def;

    output_ << "(module\n";

    for (auto& literal: module_def.string_literals)
    {
        literal.start_address = allocate_data(literal.literal);
    }

    for (const auto &import_def : module_def.imports)
    {
        emit(*import_def);
    }

    output_ << "  (memory (;0;) 2)\n";
    output_ << "  (data (i32.const 100) \"" << data_buffer << "\")\n";
    output_ << "  (global $heap_ptr (mut i32) (i32.const 1024))\n";
    output_ << "  (export \"memory\" (memory 0))\n";
    output_ << "  (export \"_start\" (func $_start))\n";
    output_ << "  (func $cabi_realloc (param $originalPtr i32)\n";
    output_ << "                      (param $originalSize i32)\n";
    output_ << "                      (param $alignment i32)\n";
    output_ << "                      (param $newSize i32)\n";
    output_ << "    (result i32)\n";

    output_ << "      global.get $heap_ptr\n";
    output_ << "      global.get $heap_ptr\n";
    output_ << "      local.get $newSize\n";
    output_ << "      i32.add\n";
    output_ << "      global.set $heap_ptr\n";

    output_ << "  )\n";

    for (const auto &func_def : module_def.functions)
    {
        emit(*func_def);
    }

    output_ << ")\n";

    current_module = nullptr;
}

void emitter::emit(const node_import_definition &import_def)
{
    output_ << "  (import \"" << import_def.ns_name << "\" ";
    // TODO: this is really ugly, but unfortunately, the "plain" name is only needed
    // in the import definition of WAT
    output_ << "\"" << std::get<node_function_head>(import_def.function_head->value).name << "\" ";

    emit(*import_def.function_head);
    output_ << "))\n";
}

void emitter::emit(const node_function_head &function_head)
{
    emit_function_signature(function_head.name, function_head.signature);
}

void emitter::emit(const node_function_definition &func_def)
{
    output_ << "  ";
    emit(*func_def.function_head);
    output_ << "\n";

    for (auto& [identifier, symbol] : func_def.function_scope->symbol_table)
    {
        if (symbol.kind == symbol_kind::local_var)
        {
            if (t_t::is_of<t_t::string>(symbol.symbol_type))
            {
                output_ << "      (local " << create_wasm_name(identifier) << "_ptr i32)\n";
                output_ << "      (local " << create_wasm_name(identifier) << "_size i32)\n";
            }
            else
            {
                output_ << "      (local " << create_wasm_name(identifier) << " i32)\n";
            }
        }
    }
    output_ << "      (local $_rec_ptr i32)\n"; // Pointer used during record initialisations

    for (const auto &expression : func_def.code)
    {
        emit(*expression);
    }

    output_ << "  )\n";
}

void emitter::emit(const node_record_definition& record_def)
{
}

void emitter::emit(const node_if_expression& if_expr)
{
    emit(*if_expr.condition);
    output_ << "      if\n";
    for (const auto& expression : if_expr.then_code)
    {
        emit(*expression);
    }
    if (!if_expr.then_code.empty())
    {
        output_ << "      else\n";
        for (const auto& expression : if_expr.else_code)
        {
            emit(*expression);
        }
    }
    output_ << "      end\n";
}

void emitter::emit(const node_function_call &func_call)
{
    for (const auto &param : func_call.parameter)
    {
        emit(*param);
    }
    output_ << "      call " << create_wasm_name(func_call.function_name) << "\n";
}

void emitter::emit(const node_let_expression& let_expression)
{
    emit(*let_expression.init_expression);
    if (t_t::is_of<t_t::string>(let_expression.symbol_ref.symbol_type))
    {
        output_ << "      local.set " << create_wasm_name(let_expression.symbol_ref.name + "_ptr") << "\n";
        output_ << "      local.set " << create_wasm_name(let_expression.symbol_ref.name + "_size") << "\n";
    }
    else
    {
        output_ << "      local.set " << let_expression.symbol_ref.index_addr << "\n";
    }
}

void emitter::emit(const node_record_initialisation& record_init)
{
    size_t record_size = type_size(record_init.type_spec);
    output_ << "      i32.const 0\n";                     // originalPtr
    output_ << "      i32.const 0\n";                     // originalSize
    output_ << "      i32.const 4\n";                     // alignment
    output_ << "      i32.const " << record_size << "\n"; // newSize
    output_ << "      call $cabi_realloc\n";
    output_ << "      local.set $_rec_ptr\n";

    // TODO: semantic check should make sure, that the type_spec really is a record
    auto& record_type = std::get<t_t::record>(record_init.type_spec);

    size_t offset = 0;
    // TODO: make the inverse check if any field init tries to init a field that isn't
    // in the definition
    for (const auto& field : record_type.fields)
    {
        // get the address for the field to initialise
        output_ << "      local.get $_rec_ptr\n";

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
        output_ << "      i32.store offset=" << offset << "\n";

        offset += type_size(*field.type_spec);
    }

    // after record initialisation, leave the pointer of the newly created record
    // on the stack
    output_ << "      local.get $_rec_ptr\n";
}

void emitter::emit(const node_field_deref& field_deref)
{
    if (t_t::is_of<t_t::string>(field_deref.object_type))
    {
        // TODO: for the moment we can only handle variables of types strings
        // but not strings that are fields of records
        auto var_name = std::get<node_var_reference>(field_deref.object->value).name;
        output_ << "      local.get $" << var_name << "_" << field_deref.fieldname << "\n";
    }
    else
    {
        emit(*field_deref.object);
        auto maybe_size = record_field_offset(field_deref.object_type, field_deref.fieldname);
        assert(maybe_size.has_value() && "A record field should have its offset calculated by this point");

        output_ << "      i32.load offset=" << maybe_size.value() << "\n";
    }
}

void emitter::emit(const node_expression &root)
{
    emit(*root.left);
    emit(*root.right);
    switch (root.operation)
    {
    case op_minus:
        output_ << "      i32.sub\n";
        break;
    case op_plus:
        output_ << "      i32.add\n";
        break;
    case op_multiply:
        output_ << "      i32.mul\n";
        break;
    case op_division:
        output_ << "      i32.div_u\n";
        break;
    case op_modulus:
        output_ << "      i32.rem_u\n";
        break;
    case op_assignment:
        if (std::holds_alternative<node_pointer_deref>(root.left->value))
        {
            if (t_t::is_of<t_t::u8>(assigned_node_type(*root.left)))
            {
                output_ << "      i32.store8\n";
                break;
            }
            else
            {
                output_ << "      i32.store\n";
                break;
            }
        }
        else
        {
            output_ << "      local.set " << std::get<node_var_reference>(root.left->value).symbol_ref.index_addr << "\n";
            break;
        }
    case op_conversion:
        if ((t_t::is_of<t_t::void_type>(assigned_node_type(*root.right))) &&
            (!t_t::is_of<t_t::void_type>(assigned_node_type(*root.left))))
        {
            output_ << "      drop\n";
        }
        break;
    }
}

void emitter::emit(const node_string_literal& literal)
{
    uint32_t ptr = current_module->string_literals[literal.table_index].start_address;
    output_ << "      i32.const " << literal.size << "\n";
    output_ << "      i32.const " << ptr << "\n";
}

void emitter::emit(const node_char_literal& literal)
{
    output_ << "      i32.const " << literal.ch << "\n";
}


void emitter::emit(const node_type_spec& spec)
{
}

void emitter::emit(const node_bool_const& bool_const)
{
    output_ << "      i32.const " << (bool_const.value ? "1" : "0") << "\n";
}

void emitter::emit(const node_number &number)
{
    output_ << "      i32.const " << number.number << "\n";
}

void emitter::emit(const node_var_reference &variable)
{
    // When the variable is used in an LHS context, then we need to
    // handle the storing of the value differently
    if (variable.context == assign_context::rhs)
    {
        output_ << "      local.get " << variable.symbol_ref.index_addr << "\n";
    }
}

void emitter::emit(const node_pointer_deref& ptr_deref)
{
    emit(*ptr_deref.pointer_expression);
    if (ptr_deref.context == assign_context::rhs)
    {
        if (t_t::is_of<t_t::u8>(ptr_deref.assigned_type))
        {
            output_ << "      i32.load8_u\n";
        }
        else
        {
            output_ << "      i32.load\n";
        }
    }
    else
    {
        // in LHS context we have to get address, which in our
        // case means to get the index of the variable
        auto var_index = std::get<node_var_reference>(ptr_deref.pointer_expression->value).symbol_ref.index_addr;
        output_ << "      local.get " << var_index << "\n";
    }
}

void emitter::emit_function_signature(const std::string &function_name, const function_signature &signature)
{
    output_ << "(func " << create_wasm_name(function_name);

    for (const auto &param : signature.parameters)
    {
        output_ << " (param $" << param.name << " " << type_mapping(param.type_spec) << ")";
    }

    if (!t_t::is_of<t_t::void_type>(signature.return_type))
    {
        output_ << " (result " << type_mapping(signature.return_type) << ")";
    }
}

uint32_t emitter::allocate_data(const std::string& data)
{
    auto alloc_address = data_offset;
    data_offset += data.size();

    for (const char& c : data) {
        switch (c) {
            case '\\':
                data_buffer += "\\\\";
                break;
            case '\'':
                data_buffer += "\\\'";
                break;
            case '\"':
                data_buffer += "\\\"";
                break;
            case '\t':
                data_buffer += "\\t";
                break;
            case '\r':
                data_buffer += "\\r";
                break;
            case '\n':
                data_buffer += "\\n";
                break;
            default:
                if (c>='\x20')
                {
                    data_buffer += std::string(1, c);
                }
                else
                {
                    std::ostringstream oss;
                    oss << "\\" << std::uppercase << std::hex << std::setw(2) << std::setfill('0') 
                        << ((unsigned int)c & 0xff);
                    data_buffer +=  oss.str();
                }
        }
    }
    return alloc_address;
}
