#include <catch2/catch_test_macros.hpp>
#include <sstream>

struct root
{
};

using custom_type = int;
// definition of a simple "context" type that we will use to test
// context-based dumping of scalar types.
struct context
{
    std::string to_string(custom_type value)
    {
        return "Value=" + std::to_string(value);
    }
};

#define DEFINE_NODES
#include "../dumpable.hpp"
#include "nodes_to_test.hpp"
#undef DEFINE_NODES

#define DEFINE_DUMP_FUNCS
#define DUMPER_CTX_TYPE context*
#define DUMPER_CTX_DUMP_FUNC(cntxt, val) cntxt->to_string(val)
#include "../dumpable.hpp"
#include "nodes_to_test.hpp"
#undef DEFINE_DUMP_FUNCS

TEST_CASE("simple struct with scalar")
{
    std::stringstream output;

    tst_single_scalar sut{.name = "value"};
    dump_tst(output, nullptr, sut, 0);

    REQUIRE(output.str() == "    _type: single_scalar\n"
                            "    name: value\n");
}

TEST_CASE("struct with no_dump field")
{
    std::stringstream output;

    tst_no_dump_fields sut{.name1 = "text", .value = 43, .name2 = "asdf"};
    dump_tst(output, nullptr, sut, 0);

    REQUIRE(output.str() == "    _type: no_dump_fields\n"
                            "    name1: text\n"
                            "    name2: asdf\n");
}

TEST_CASE("struct with nested struct")
{
    std::stringstream output;

    tst_single_nesting sut{
        .child = make_node(tst_single_scalar{.name = "value"})
    };
    dump_tst(output, nullptr, sut, 0);

    REQUIRE(output.str() == "    _type: single_nesting\n"
                            "    child:\n"
                            "        _type: single_scalar\n"
                            "        name: value\n");
}

TEST_CASE("struct with vector of structs")
{
    std::stringstream output;

    std::vector<tst_single_scalar> child;
    child.push_back(tst_single_scalar{.name = "value1"});
    child.push_back(tst_single_scalar{.name = "value2"});

    tst_struct_with_vector sut{
        .child = std::move(child)
    };
    dump_tst(output, nullptr, sut, 0);

    REQUIRE(output.str() == "    _type: struct_with_vector\n"
                            "    child:\n"
                            "      - _type: single_scalar\n"
                            "        name: value1\n"
                            "      - _type: single_scalar\n"
                            "        name: value2\n");
}

TEST_CASE("struct with vector of struct pointers")
{
    std::stringstream output;

    std::vector<std::unique_ptr<tst_single_scalar>> child;
    child.push_back(make_node(tst_single_scalar{.name = "value1"}));
    child.push_back(make_node(tst_single_scalar{.name = "value2"}));

    tst_struct_with_struct_list sut{
        .child = std::move(child)
    };
    dump_tst(output, nullptr, sut, 0);

    REQUIRE(output.str() == "    _type: struct_with_struct_list\n"
                            "    child:\n"
                            "      - _type: single_scalar\n"
                            "        name: value1\n"
                            "      - _type: single_scalar\n"
                            "        name: value2\n");
}

TEST_CASE("dumping of context-based field")
{
    std::stringstream output;

    context ctx;
    tst_struct_with_context_value sut{
        .child = custom_type(42)
    };
    dump_tst(output, &ctx, sut, 0);

    REQUIRE(output.str() == "    _type: struct_with_context_value\n"
                            "    child: Value=42\n");
}