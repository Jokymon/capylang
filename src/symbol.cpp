#include "symbol.hpp"
#include <cassert>
#include <ranges>

bool function_type::is_call_signature_eq(const function_type& other)
{
    if (parameter_types.size() != other.parameter_types.size())
    {
        return false;
    }
    for (auto [this_id, other_id]: std::views::zip(parameter_types, other.parameter_types))
    {
        if (this_id != other_id)
        {
            return false;
        }
    }
    return true;
}

std::string function_type::repr_call_sig(const context& ctx) const
{
    std::string r = "(";
    if (parameter_types.size()>0)
    {
        r += ctx.repr(parameter_types[0]);

        size_t index = 1;
        while (index<parameter_types.size())
        {
            r += ", " + ctx.repr(parameter_types[index]);
            index++;
        }
    }

    r += ")";
    return r;
}

context::context()
{
    // reserve entry for ILLEGAL_TYPE
    types.emplace_back(type_var{});
}

type_id context::intern_primitive(primitive_type p_type)
{
    type_kind2 kind = p_type;
    auto it = interned.find(kind);
    if (it != interned.end())
    {
        return it->second;
    }

    type_id id = static_cast<type_id>(types.size());
    types.emplace_back(kind);
    interned.emplace(kind, id);

    return id;
}

type_id context::intern(const type_kind2& type)
{
    auto it = interned.find(type);
    if (it != interned.end())
    {
        return it->second;
    }

    type_id id = static_cast<type_id>(types.size());
    types.emplace_back(type);
    interned.emplace(type, id);

    return id;
}

type_id context::create_type_var()
{
    type_id id = static_cast<type_id>(types.size());
    types.emplace_back(type_var{std::nullopt});
    return id;
}

bool context::is_primitive_type(type_id type_idx, primitive_type p_type)
{
    auto t = types[type_idx];
    if (!std::holds_alternative<type_kind2>(t))
        return false;
    
    auto kind = std::get<type_kind2>(t);
    if (!std::holds_alternative<primitive_type>(kind))
        return false;

    return std::get<primitive_type>(kind) == p_type;
}

bool context::is_record_type(type_id type_idx)
{
    auto t = types[type_idx];
    if (!std::holds_alternative<type_kind2>(t))
        return false;

    auto kind = std::get<type_kind2>(t);
    return std::holds_alternative<record_type>(kind) || 
        (std::holds_alternative<primitive_type>(kind) &&
            std::get<primitive_type>(kind) == primitive_type::String);
}

bool context::is_pointer_type(type_id type_idx)
{
    auto t = types[type_idx];
    if (!std::holds_alternative<type_kind2>(t))
        return false;

    auto kind = std::get<type_kind2>(t);
    return std::holds_alternative<pointer_type>(kind);
}

bool context::is_type_var(type_id type_idx)
{
    auto t = types[type_idx];
    return std::holds_alternative<type_var>(t);
}

std::optional<type_id> context::record_field_type(type_id record_type_idx, const std::string& field_name)
{
    auto t = types[record_type_idx];
    if (!std::holds_alternative<type_kind2>(t))
        return std::nullopt;

    auto kind = std::get<type_kind2>(t);
    if (std::holds_alternative<record_type>(kind))
    {
        const record_type& r = std::get<record_type>(kind);
        for (const auto& field : r.fields)
        {
            if (field.first == field_name)
            {
                return field.second;
            }
        }
    }
    else if (std::holds_alternative<primitive_type>(kind))
    {
        if (std::get<primitive_type>(kind) == primitive_type::String)
        {
            if (field_name == "size")
            {
                return intern_primitive(primitive_type::U32);
            }
            else if (field_name == "ptr")
            {
                return intern(pointer_type{intern_primitive(primitive_type::U8)});
            }
        }
    }

    return std::nullopt;
}

std::optional<type_id> context::function_return_type(type_id function_type_idx)
{
    auto t = types[function_type_idx];
    if (!std::holds_alternative<type_kind2>(t))
        return std::nullopt;

    auto kind = std::get<type_kind2>(t);
    if (std::holds_alternative<function_type>(kind))
    {
        const function_type& f = std::get<function_type>(kind);
        return f.return_type;
    }

    return std::nullopt;
}

std::string context::repr(type_id type_idx) const
{
    if ((type_idx == ILLEGAL_TYPE) || (static_cast<size_t>(type_idx) >= types.size()))
    {
        return "!unassigned";
    }

    const auto &typ = types[type_idx];
    return std::visit([&](const auto &t) -> std::string {
        using T = std::decay_t<decltype(t)>;

        if constexpr (std::is_same_v<T, type_kind2>)
        {
            return std::visit([&](const auto &k) -> std::string {
                using K = std::decay_t<decltype(k)>;

                if constexpr (std::is_same_v<K, primitive_type>)
                {
                    switch (k) {
                        case primitive_type::Void:
                            return "void";
                        case primitive_type::Boolean:
                            return "bool";
                        case primitive_type::Char:
                            return "char";
                        case primitive_type::U8:
                            return "u8";
                        case primitive_type::U16:
                            return "u16";
                        case primitive_type::U32:
                            return "u32";
                        case primitive_type::S8:
                            return "s8";
                        case primitive_type::S16:
                            return "s16";
                        case primitive_type::S32:
                            return "s32";
                        case primitive_type::String:
                            return "string";
                    }
                }
                else if constexpr (std::is_same_v<K, pointer_type>)
                {
                    return repr(k.to) + "*";
                }
                else if constexpr (std::is_same_v<K, record_type>)
                {
                    std::string r = "record(";
                    for (const auto& field : k.fields)
                    {
                        r += field.first + ":";
                        r += repr(field.second) + ",";
                    }
                    r += ")";
                    return r;
                }
                else if constexpr (std::is_same_v<K, function_type>)
                {
                    std::string r = "func";
                    r += k.repr_call_sig(*this);
                    r += " -> " + repr(k.return_type);
                    return r;
                }
            }, t);
        }
        else if constexpr (std::is_same_v<T, type_var>)
        {
            return "type_var";
        }
    }, typ);
}

t_t::pointer::pointer(const type_kind& base_type)
: base_type(make_unique<type_kind>(base_type))
{
}

t_t::pointer::pointer(const t_t::pointer& other)
: base_type(make_unique<type_kind>(*other.base_type))
{
}

t_t::pointer& t_t::pointer::operator=(const t_t::pointer& other)
{
    base_type = make_unique<type_kind>(*other.base_type);
    return *this;
}

bool t_t::pointer::operator==(const pointer& other) const
{
    return *base_type == *other.base_type;
}

t_t::record::field_spec::field_spec(const std::string& name, std::unique_ptr<type_kind> type_spec)
: name(name)
, type_spec(std::make_unique<type_kind>(*type_spec))
{
}

t_t::record::field_spec::field_spec(const t_t::record::field_spec& other)
: name(other.name)
, type_spec(std::make_unique<type_kind>(*other.type_spec))
{
}

t_t::record::field_spec& t_t::record::field_spec::operator=(const t_t::record::field_spec& other)
{
    name = other.name;
    type_spec = make_unique<type_kind>(*other.type_spec);
    return *this;
}

t_t::record::record(const std::vector<t_t::record::field_spec>& fields)
: fields(fields)
{
}

t_t::record::record(const t_t::record& other)
: fields(other.fields)
{
}

t_t::record& t_t::record::operator=(const record& other)
{
    fields = other.fields;
    return *this;
}

bool t_t::record::operator==(const record& other) const
{
    return true;
}

std::optional<type_kind> t_t::record::field_type(const std::string& name)
{
    for (const auto& field : fields)
    {
        if (field.name == name)
        {
            return *field.type_spec;
        }
    }
    return std::nullopt;
}

scope* scope::get_global_scope() const
{
    scope* scope_iter = const_cast<scope*>(this);
    while (scope_iter->parent != nullptr)
    {
        scope_iter = scope_iter->parent;
    }
    return scope_iter;
}

std::optional<std::reference_wrapper<symbol>> scope::lookup(const std::string &name)
{
    if (symbol_table.find(name) != symbol_table.end())
    {
        return symbol_table[name];
    }
    else if (parent != nullptr)
    {
        return parent->lookup(name);
    }
    else
    {
        return std::nullopt;
    }
}

std::optional<type_id> scope::lookup_type2(const std::string& name)
{
    if (type_table2.find(name) != type_table2.end())
    {
        return type_table2[name];
    }
    else if (parent != nullptr)
    {
        return parent->lookup_type2(name);
    }
    else
    {
        return std::nullopt;
    }
}

std::optional<std::reference_wrapper<symbol>> scope::lookup_function(const std::string &name)
{
    if ((symbol_table.find(name) != symbol_table.end()) && (symbol_table[name].kind == symbol_kind::function))
    {
        return symbol_table[name];
    }
    else if (parent != nullptr)
    {
        return parent->lookup_function(name);
    }
    else
    {
        return std::nullopt;
    }
}
