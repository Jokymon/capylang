#include "symbol.hpp"
#include <cassert>

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

bool context::is_primitive_type(type_id type_idx, primitive_type p_type)
{
    auto t = types[type_idx];
    if (!std::holds_alternative<primitive_type>(t))
        return false;

    return std::get<primitive_type>(t) == p_type;
}

bool context::is_record_type(type_id type_idx)
{
    auto t = types[type_idx];
    return std::holds_alternative<record_type>(t) || 
        (std::holds_alternative<primitive_type>(t) &&
            std::get<primitive_type>(t) == primitive_type::String);
}

bool context::is_pointer_type(type_id type_idx)
{
    auto t = types[type_idx];
    return std::holds_alternative<pointer_type>(t);
}

std::optional<type_id> context::record_field_type(type_id record_type_idx, const std::string& field_name)
{
    auto t = types[record_type_idx];
    if (std::holds_alternative<record_type>(t))
    {
        const record_type& r = std::get<record_type>(t);
        for (const auto& field : r.fields)
        {
            if (field.first == field_name)
            {
                return field.second;
            }
        }
    }
    else if (std::holds_alternative<primitive_type>(t))
    {
        if (std::get<primitive_type>(t) == primitive_type::String)
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

std::string context::repr(type_id type_idx) const
{
    auto typ = types[type_idx];
    return std::visit([&](const auto &t) -> std::string {
        using T = std::decay_t<decltype(t)>;

        if constexpr (std::is_same_v<T, primitive_type>)
        {
            switch (t) {
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
        else if constexpr (std::is_same_v<T, pointer_type>)
        {
            return repr(t.to) + "*";
        }
        else if constexpr (std::is_same_v<T, record_type>)
        {
            std::string r = "record(";
            for (const auto& field : t.fields)
            {
                r += field.first + ":";
                r += repr(field.second) + ",";
            }
            r += ")";
            return r;
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

type_kind t_t::from_new_style(const context& ctx, type_id idx)
{
    auto typ = ctx.types.at(idx);
    return std::visit([&](const auto& t) -> type_kind {
        using T = std::decay_t<decltype(t)>;

        if constexpr (std::is_same_v<T, primitive_type>)
        {
            switch (t) {
                case primitive_type::Void:
                    return t_t::void_type{};
                case primitive_type::Char:
                    return t_t::char_type{};
                case primitive_type::Boolean:
                    return t_t::boolean{};
                case primitive_type::U8:
                    return t_t::u8{};
                case primitive_type::U16:
                    return t_t::u16{};
                case primitive_type::U32:
                    return t_t::u32{};
                case primitive_type::S8:
                    return t_t::s8{};
                case primitive_type::S16:
                    return t_t::s16{};
                case primitive_type::S32:
                    return t_t::s32{};
                case primitive_type::String:
                    return t_t::string{};
            }
        }
        else if constexpr (std::is_same_v<T, pointer_type>)
        {
            auto base_type = t_t::from_new_style(ctx, t.to);
            return t_t::pointer{base_type};
        }
        else if constexpr (std::is_same_v<T, record_type>)
        {
            std::vector<record::field_spec> fields;
            for (const auto& f : t.fields)
            {
                fields.emplace_back(record::field_spec{
                    f.first,
                    std::make_unique<type_kind>(t_t::from_new_style(ctx, f.second))
                });
            }
            return t_t::record(fields);
        }
    },
    typ);
}

std::string repr_type(const type_kind& type_spec)
{
    return std::visit([&](const auto &t) -> std::string {
        using T = std::decay_t<decltype(t)>;

        if constexpr (std::is_same_v<T, t_t::unassigned>)
            return "!unassigned";
        else if constexpr (std::is_same_v<T, t_t::void_type>)
            return "void";
        else if constexpr (std::is_same_v<T, t_t::boolean>)
            return "bool";
        else if constexpr (std::is_same_v<T, t_t::s32>)
            return "s32";
        else if constexpr (std::is_same_v<T, t_t::u8>)
            return "u8";
        else if constexpr (std::is_same_v<T, t_t::u32>)
            return "u32";
        else if constexpr (std::is_same_v<T, t_t::string>)
            return "string";
        else if constexpr (std::is_same_v<T, t_t::pointer>) {
            if (!t.base_type) {
                assert(false && "The base type of a pointer has an unexpected nullptr reference");
                return "*null*";
            }
            return repr_type(*t.base_type) + "*";
        }
        else if constexpr (std::is_same_v<T, t_t::record>) {
            std::string repr = "record(";
            for (const auto& field: t.fields) {
                repr += field.name + ":";
                repr += repr_type(*field.type_spec) + ",";
            }
            repr += ")";
            return repr;
        }
        else {
            return "UNEXPECTED BRANCH";
        }
    }, type_spec);
}

bool function_signature::equals_call_signature(function_signature &other)
{
    if (parameters.size() != other.parameters.size())
    {
        return false;
    }

    for (size_t param_index = 0; param_index < parameters.size(); param_index++)
    {
        if (parameters[param_index].type_spec != other.parameters[param_index].type_spec)
        {
            return false;
        }
    }

    return true;
}

std::string function_signature::repr()
{
    std::string r = "(";
    if (parameters.size()>0)
    {
        r += repr_type(parameters[0].type_spec);

        size_t index = 1;
        while (index<parameters.size())
        {
            r += ", " + repr_type(parameters[index].type_spec);
            index++;
        }
    }
    r += ")";
    return r;
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

std::optional<type_kind> scope::lookup_type(const std::string& name)
{
    if (type_table.find(name) != type_table.end())
    {
        return type_table[name];
    }
    else if (parent != nullptr)
    {
        return parent->lookup_type(name);
    }
    else
    {
        return std::nullopt;
    }
}

std::optional<type_id> scope::lookup_type2(const std::string& name)
{
    if (type_table.find(name) != type_table.end())
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

std::optional<symbol> scope::lookup_function(const std::string &name)
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
