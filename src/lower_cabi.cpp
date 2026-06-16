#include "lower_cabi.hpp"
#include "tools.hpp"
#include <ranges>

lower_cabi::lower_cabi(context& ctx)
: parse_context(ctx)
{
}

void lower_cabi::lower_function_arguments(const node_function_head& func_head, arguments_type& args)
{
    auto function_type_entry = parse_context.types[to_index(func_head.signature.function_type)];
    auto func_type = std::get<function_type>(std::get<type_kind>(function_type_entry));

    const auto& parameters = func_head.signature.parameters;
    for (auto [param, param_typ] : std::views::zip(parameters, func_type.parameter_types))
    {
        type_id resolved_param_typ = parse_context.resolved_type(param_typ);
        CAPY_ASSERT(parse_context.is_resolved(resolved_param_typ), "In emitter stage, all required types should be resolved");

        const auto& type_entry = parse_context.types[to_index(resolved_param_typ)];
        // when the type is resolved, we know that it can't be a type variable,
        // let's still make sure it really is so
        CAPY_ASSERT(std::holds_alternative<type_kind>(type_entry), "Unexpected type variable in resolved type");
        const auto& type_spec = std::get<type_kind>(type_entry);

        lower_function_argument(type_spec, param.name, args);
    }
}

void lower_cabi::lower_function_argument(const type_kind& ty, const std::string& basename, arguments_type& args)
{
    std::visit(
        [&](const auto& s) -> void
        {
            using K = std::decay_t<decltype(s)>;

            if constexpr (std::is_same_v<K, primitive_type>)
            {
                switch (s)
                {
                    case primitive_type::Void:
                        args.push_back({basename, wasm_type::none});
                        break;
                    case primitive_type::Char:
                    case primitive_type::S8:
                        args.push_back({basename, wasm_type::i8});
                        break;
                    case primitive_type::S16:
                        args.push_back({basename, wasm_type::i16});
                        break;
                    case primitive_type::S32:
                        args.push_back({basename, wasm_type::i32});
                        break;
                    case primitive_type::U8:
                        args.push_back({basename, wasm_type::u8});
                        break;
                    case primitive_type::U16:
                        args.push_back({basename, wasm_type::u16});
                        break;
                    case primitive_type::U32:
                        args.push_back({basename, wasm_type::u32});
                        break;
                    case primitive_type::Boolean:
                        args.push_back({basename, wasm_type::i32});
                        break;
                    case primitive_type::String:
                        args.push_back({basename + "_ptr", wasm_type::i32});
                        args.push_back({basename + "_size", wasm_type::i32});
                        break;
                    default:
                        CAPY_FAIL("Unknown primitive type in function argument lowering");
                        break;
                }
            }
            else if constexpr (std::is_same_v<K, pointer_type>)
            {
                args.push_back({basename, wasm_type::i32});
            }
            // TODO: implement record type passing
            // TODO: for much later also consider function passing
            else
            {
                CAPY_FAIL("Unhandled type in function argument lowering");
            }
        },
        ty
    );
}

void lower_cabi::lower_local_variables(const node_function_definition& func_def, wasm_function& func)
{
    for (const auto& [identifier, symbol_id] : func_def.function_scope->symbol_table)
    {
        const auto& symbol = parse_context.symbol_at(symbol_id);

        if (symbol.kind == symbol_kind::local_var)
        {
            lower_local_variable(symbol, func);
        }
    }
    // TODO: this is still a somewhat awkward solution for the moment; we use
    // this implicitly created local variable to hold a newly allocated record.
    // However this will very likely cause problems as soon as we start
    // supporting nested record initialisations.
    func.allocate_local("_rec_ptr", wasm_type::i32);
}

void lower_cabi::lower_local_variable(const symbol& sym, wasm_function& func)
{
    if (parse_context.is_primitive_type(sym.symbol_type, primitive_type::String))
    {
        func.allocate_local((sym.name + "_ptr").c_str(), wasm_type::i32);
        func.allocate_local((sym.name + "_size").c_str(), wasm_type::i32);
    }
    else
    {
        type_id var_type = parse_context.resolved_type(sym.symbol_type);
        const auto& type_entry = parse_context.types[to_index(var_type)];
        if (!std::holds_alternative<type_kind>(type_entry))
        {
            printf("Checking variable %s\n", sym.name.c_str());
            CAPY_FAIL("Encountered unresolved type variable in lowering to CABI");
        }

        const auto& type_spec = std::get<type_kind>(type_entry);

        std::visit([&](const auto& k) -> void
                   {
                    using K = std::decay_t<decltype(k)>;

                    if constexpr (std::is_same_v<K, primitive_type>) {
                        switch (k) {
                            case primitive_type::Void:
                                func.allocate_local(sym.name.c_str(), wasm_type::none);
                                break;
                            case primitive_type::Char:
                            case primitive_type::S8:
                                func.allocate_local(sym.name.c_str(), wasm_type::i8);
                                break;
                            case primitive_type::S16:
                                func.allocate_local(sym.name.c_str(), wasm_type::i16);
                                break;
                            case primitive_type::S32:
                                func.allocate_local(sym.name.c_str(), wasm_type::i32);
                                break;
                            case primitive_type::U8:
                                func.allocate_local(sym.name.c_str(), wasm_type::u8);
                                break;
                            case primitive_type::U16:
                                func.allocate_local(sym.name.c_str(), wasm_type::u16);
                                break;
                            case primitive_type::U32:
                                func.allocate_local(sym.name.c_str(), wasm_type::u32);
                                break;
                            case primitive_type::Boolean:
                                func.allocate_local(sym.name.c_str(), wasm_type::i32);
                                break;
                            case primitive_type::String:
                                func.allocate_local(sym.name.c_str(), wasm_type::i32);
                                break;
                            default:
                                CAPY_FAIL("Unknown primitive type in lowering local variable");
                                break;
                        }
                    } else if constexpr (std::is_same_v<K, pointer_type>) {
                        func.allocate_local(sym.name.c_str(), wasm_type::i32);
                    } else if constexpr (std::is_same_v<K, record_type>) {
                        // TODO: decide whether fields should be local vars or in linear memory based on the number and
                        // the size of the fields
                        size_t index = 0;
                        for (const auto& field : k.fields)
                        {
                            // TODO: allocate the correct data type for the variable
                            func.allocate_local((sym.name + "_" + std::to_string(index)).c_str(), wasm_type::i32);
                            index++;
                        }
                    } else if constexpr (std::is_same_v<K, function_type>) {
                        printf("Requested function type in conversion\n");
                        func.allocate_local(sym.name.c_str(), wasm_type::none);
                    } else {
                        CAPY_FAIL("Unknown type kind in from_type_kind");
                    } },
                   type_spec);
    }
}


void lower_cabi::lower_variable_ref_read(symbol_id symbol_ref, wasm_block& output_block)
{
    const auto& symbol = parse_context.symbol_at(symbol_ref);

    // TODO: In case of record types, here we need to know whether the record is available as function parameter
    // or as locally created record
    if (parse_context.is_primitive_type(symbol.symbol_type, primitive_type::String))
    {
        if (symbol.kind == symbol_kind::global_var)
        {
            output_block.global_get((symbol.name + "_ptr").c_str());
            output_block.global_get((symbol.name + "_size").c_str());
        }
        else
        {
            output_block.local_get((symbol.name + "_ptr").c_str());
            output_block.local_get((symbol.name + "_size").c_str());
        }
    }
    else
    {
        if (symbol.kind == symbol_kind::global_var)
        {
            output_block.global_get(symbol.name.c_str());
        }
        else
        // TODO: handle symbol_kind::argument differently in particular for records
        {
            output_block.local_get(symbol.name.c_str());
        }
    }
}

void lower_cabi::lower_variable_ref_write(symbol_id symbol_ref, wasm_block& output_block)
{
    const auto& symbol = parse_context.symbol_at(symbol_ref);

    if (parse_context.is_primitive_type(symbol.symbol_type, primitive_type::String))
    {
        output_block.local_set((symbol.name + "_size").c_str());
        output_block.local_set((symbol.name + "_ptr").c_str());
    }
    else
    {
        output_block.local_set(symbol.name.c_str());
    }
}
