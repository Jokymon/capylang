#ifndef DUMPABLE_HPP
#define DUMPABLE_HPP
#include <cmath>

template <class T>
std::unique_ptr<T> make_node(T value)
{
    return std::make_unique<T>(std::move(value));
}

// The dumpable tooling aims at supporting factored out data members into common
// base classes. For dumping we just assume that this factored out members are
// normal members of the derived type.
// To make this work, we explicitly mark dumpable "base" classes with a static
// constexpr boolean class member named `is_based` which is set to true. Non-
// base classes either have set this value to false or do not have this member
// at call.
// To be able to use this property, we have to defer access to this member far
// down in the compile process and use the combination of get_is_base() together
// with the helper function dump_parent_if_base().
template <typename T>
constexpr bool get_is_base()
{
    if constexpr (requires { { T::is_base } -> std::convertible_to<bool>; })
    {
        return T::is_base;
    }
    else
    {
        return false;
    }
}

template <typename T>
constexpr bool is_base_v = get_is_base<T>();

template <typename Parent, typename Derived, typename DumpParentFn>
void dump_parent_if_base(const Derived& def, DumpParentFn&& dump_parent)
{
    if constexpr (is_base_v<Parent>)
    {
        dump_parent(static_cast<const Parent&>(def));
    }
}

#endif

// The API of dumpable provides the following macros to define a
// dumpable node structure:
//
// DEF_NODE(namespace, node_name, parent class)
//      Starting definition of a dumpable node
// DEF_BASE_NODE(namespace, node_name, parent class)
//      Starting definition of a base class which contains members that should
//      also be dumped when dumping a derived class
// DEF_END
//      Finishing definition of a dumpable node
// DEF_NO_DUMP_PLAIN(text)
// DEF_NO_DUMP(type, name)
// DEF_SCALAR_FIELD(type, name)
// DEF_SCALAR_WITH_CONTEXT(type, name)
// DEF_NODE_FIELD(namespace, type, name)
// DEF_LIST_FIELD(namespace, type, name)
// DEF_NODE_LIST_FIELD(namespace, type, name)
//
// To create the datatype definitions, include your header file with
// the structure after setting the definition
//
//      #define DEFINE_NODES
//
// then, possibly in a different file, for the implementation of the
// dump functions, incldue the datatype definitions again
// after setting
//
//      #define DEFINE_DUMP_FUNCS

#ifdef DEFINE_NODES

#define DEF_NODE(node_ns, node_name, parent)     \
    struct node_ns##_##node_name : public parent \
    {                                            \
        static constexpr bool is_base = false;

#define DEF_BASE_NODE(node_ns, node_name, parent) \
    struct node_ns##_##node_name : public parent  \
    {                                             \
        static constexpr bool is_base = true;

#define DEF_END \
    }           \
    ;

#define DEF_NO_DUMP(type, name) \
    type name;

#define DEF_NO_DUMP_PLAIN(text) \
    text

#define DEF_SCALAR_FIELD(type, name) \
    type name;

#define DEF_SCALAR_WITH_CONTEXT(type, name) \
    type name;

#define DEF_NODE_FIELD(node_ns, type, name) \
    std::unique_ptr<type> name;

#define DEF_LIST_FIELD(node_ns, type, name) \
    std::vector<type> name;

#define DEF_NODE_LIST_FIELD(node_ns, type, name) \
    std::vector<std::unique_ptr<type>> name;

#elif defined(DEFINE_DUMP_FUNCS)

#ifndef DUMPER_CTX_TYPE
#error "For dumpable.hpp to work, you need to define a datatype that will be used for dumping context-based field members by setting DUMPER_CTX_TYPE"
#endif
#ifndef DUMPER_CTX_DUMP_FUNC
#error "For dumpable.hpp to work, you need to define a macro that will be used for dumping one context-based field value by setting DUMPER_CTX_DUMP_FUNC(ctx, value)"
#endif

// We use negative indentations to say that an element is to be printed as a
// list entry in a YAML output and needs a '-' prefix
// TODO: add functionality to omit the ':' at the end if the node
// definition doesn't contain any fields
#undef DEF_NODE
#define DEF_NODE(node_ns, node_name, parent)                                                                 \
    void dump_##node_ns(std::ostream& os, DUMPER_CTX_TYPE ctx, const node_ns##_##node_name& def, int indent) \
    {                                                                                                        \
        std::string ind = std::string(abs(indent), ' ');                                                     \
        std::string field_ind = std::string(4, ' ');                                                         \
        if (indent < 0)                                                                                      \
        {                                                                                                    \
            os << ind << std::string(2, ' ') << "- _type: " << #node_name << "\n";                           \
        }                                                                                                    \
        else                                                                                                 \
        {                                                                                                    \
            os << ind << field_ind << "_type: " << #node_name << "\n";                                       \
        }                                                                                                    \
        dump_parent_if_base<parent>(def, [&](const auto& base_def) { dump_##node_ns(os, ctx, base_def, indent); });

#undef DEF_BASE_NODE
#define DEF_BASE_NODE(node_ns, node_name, parent)                                                            \
    void dump_##node_ns(std::ostream& os, DUMPER_CTX_TYPE ctx, const node_ns##_##node_name& def, int indent) \
    {                                                                                                        \
        std::string ind = std::string(abs(indent), ' ');                                                     \
        std::string field_ind = std::string(4, ' ');

#undef DEF_END
#define DEF_END \
    }

#undef DEF_SCALAR_FIELD
#define DEF_SCALAR_FIELD(type, name) \
    os << ind << field_ind << #name << ": " << def.name << "\n";

#undef DEF_SCALAR_WITH_CONTEXT
#define DEF_SCALAR_WITH_CONTEXT(type, name) \
    os << ind << field_ind << #name << ": " << DUMPER_CTX_DUMP_FUNC(ctx, def.name) << "\n";

#undef DEF_NO_DUMP
#define DEF_NO_DUMP(type, name)

#undef DEF_NO_DUMP_PLAIN
#define DEF_NO_DUMP_PLAIN(text)

#undef DEF_NODE_FIELD
#define DEF_NODE_FIELD(node_ns, type, name)                  \
    os << ind << field_ind << #name << ":\n";                \
    if (def.name)                                            \
    {                                                        \
        dump_##node_ns(os, ctx, *def.name, abs(indent) + 4); \
    }

#undef DEF_LIST_FIELD
#define DEF_LIST_FIELD(node_ns, type, name)                 \
    os << ind << field_ind << #name << ":\n";               \
    for (const auto& entry : def.name)                      \
    {                                                       \
        dump_##node_ns(os, ctx, entry, -(abs(indent) + 4)); \
    }

#undef DEF_NODE_LIST_FIELD
#define DEF_NODE_LIST_FIELD(node_ns, type, name)             \
    os << ind << field_ind << #name << ":\n";                \
    for (const auto& entry : def.name)                       \
    {                                                        \
        dump_##node_ns(os, ctx, *entry, -(abs(indent) + 4)); \
    }

#else

#endif
