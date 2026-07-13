#include "lower_cabi.hpp"
#include "tools.hpp"
#include <ranges>

lower_cabi::lower_cabi(context& ctx)
: parse_context(ctx)
{
}

void lower_cabi::lower_function_arguments(const lir::function_head& func_head, arguments_type& args)
{
    auto function_type_entry = parse_context.types[to_index(func_head.signature.function_type)];
    auto func_type = std::get<function_type>(std::get<type_kind>(function_type_entry));

    const auto& parameter_names = func_head.signature.parameters;
    for (auto [param, param_typ] : std::views::zip(parameter_names, func_type.parameter_types))
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
                    default:
                        CAPY_FAIL("Unknown primitive type in function argument lowering");
                        break;
                }
            }
            else if constexpr (std::is_same_v<K, pointer_type>)
            {
                args.push_back({basename, wasm_type::i32});
            }
            else if constexpr (std::is_same_v<K, record_type>)
            {
                size_t index = 0;
                for (const auto& field : s.fields)
                {
                    type_id resolved_field_typ = parse_context.resolved_type(field.second);
                    CAPY_ASSERT(parse_context.is_resolved(resolved_field_typ), "In emitter stage, all required types should be resolved");
                    const auto& type_entry = parse_context.types[to_index(resolved_field_typ)];
                    CAPY_ASSERT(std::holds_alternative<type_kind>(type_entry), "Unexpected type variable in resolved type");
                    const auto& type_spec = std::get<type_kind>(type_entry);

                    std::string fieldname = basename + "_" + std::to_string(index);
                    lower_function_argument(type_spec, fieldname, args);
                    index++;
                }
            }
            // TODO: for much later also consider function passing
            else
            {
                CAPY_FAIL("Unhandled type in function argument lowering");
            }
        },
        ty
    );
}

void lower_cabi::lower_local_variable(const symbol& sym, wasm_function& func)
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

wasm_type lower_cabi::lower_return_type(type_id return_type)
{
    const auto& type_entry = parse_context.types[to_index(return_type)];

    return std::visit(
        [&](const auto& t) -> auto
        {
            using T = std::decay_t<decltype(t)>;

            if constexpr (std::is_same_v<T, type_kind>)
            {
                const auto& type_spec = t;

                return std::visit([&](const auto& k) -> wasm_type
                                  {
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
                    } },
                                  type_spec);
            }
            else if constexpr (std::is_same_v<T, type_var>)
            {
                // TODO: LIR passes should take care that we only have fully resolved
                // types here
                CAPY_FAIL("Found unresolved type in lowering pass");
                return wasm_type::none;
            }
        },
        type_entry
    );
}
