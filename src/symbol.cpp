#include "symbol.hpp"
#include <cassert>

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

std::optional<func_symbol> scope::lookup_function(const std::string &name)
{
    if (function_table.find(name) != function_table.end())
    {
        return function_table[name];
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
