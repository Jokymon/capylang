#ifndef DUMPABLE_HPP
#define DUMPABLE_HPP
#include <cmath>

template <class T>
std::unique_ptr<T> make_node(T value)
{
    return std::make_unique<T>(std::move(value));
}
#endif

#ifdef DEFINE_NODES

#define DEF_NODE(node_ns, node_name, parent)     \
    struct node_ns##_##node_name : public parent \
    {

#define DEF_END \
    }           \
    ;

#define DEF_NO_DUMP(type, name) \
    type name;

#define DEF_SCALAR_FIELD(type, name) \
    type name;

#define DEF_NODE_FIELD(node_ns, type, name) \
    std::unique_ptr<type> name;

#define DEF_LIST_FIELD(node_ns, type, name) \
    std::vector<type> name;

#define DEF_NODE_LIST_FIELD(node_ns, type, name) \
    std::vector<std::unique_ptr<type>> name;

#elif defined(DEFINE_DUMP_FUNCS)

// We use negative indentations to say that an element is to be printed as a
// list entry in a YAML output and needs a '-' prefix
// TODO: add functionality to omit the ':' at the end if the node
// definition doesn't contain any fields
#undef DEF_NODE
#define DEF_NODE(node_ns, node_name, parent)                                            \
    void dump_##node_ns(std::ostream& os, const node_ns##_##node_name& def, int indent) \
    {                                                                                   \
        std::string ind = std::string(abs(indent), ' ');                                \
        std::string field_ind = std::string(4, ' ');                                    \
        if (indent < 0)                                                                 \
        {                                                                               \
            os << ind << std::string(2, ' ') << "- _type: " << #node_name << "\n";      \
        }                                                                               \
        else                                                                            \
        {                                                                               \
            os << ind << field_ind << "_type: " << #node_name << "\n";                  \
        }

#undef DEF_END
#define DEF_END \
    }

#undef DEF_SCALAR_FIELD
#define DEF_SCALAR_FIELD(type, name) \
    os << ind << field_ind << #name << ": " << def.name << "\n";

#undef DEF_NO_DUMP
#define DEF_NO_DUMP(type, name)

#undef DEF_NODE_FIELD
#define DEF_NODE_FIELD(node_ns, type, name)             \
    os << ind << field_ind << #name << ":\n";           \
    if (def.name)                                       \
    {                                                   \
        dump_##node_ns(os, *def.name, abs(indent) + 4); \
    }

#undef DEF_LIST_FIELD
#define DEF_LIST_FIELD(node_ns, type, name)            \
    for (const auto& entry : def.##name)               \
    {                                                  \
        dump_##node_ns(os, entry, -(abs(indent) + 4)); \
    }

#undef DEF_NODE_LIST_FIELD
#define DEF_NODE_LIST_FIELD(node_ns, type, name)        \
    os << ind << field_ind << #name << ":\n";           \
    for (const auto& entry : def.name)                  \
    {                                                   \
        dump_##node_ns(os, *entry, -(abs(indent) + 4)); \
    }

#else

#endif