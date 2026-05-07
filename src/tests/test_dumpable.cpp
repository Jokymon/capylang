#include <catch2/catch_test_macros.hpp>
#include <sstream>

struct root
{
};

#define DEFINE_NODES
#include "../dumpable.hpp"
#include "nodes_to_test.hpp"
#undef DEFINE_NODES

#define DEFINE_DUMP_FUNCS
#include "../dumpable.hpp"
#include "nodes_to_test.hpp"
#undef DEFINE_DUMP_FUNCS

TEST_CASE("simple struct with scalar")
{
    std::stringstream output;

    tst_single_scalar sut{.name = "value"};
    dump_tst(output, sut, 0);

    REQUIRE(output.str() == "    _type: single_scalar\n"
                            "    name: value\n");
}

TEST_CASE("struct with no_dump field")
{
    std::stringstream output;

    tst_no_dump_fields sut{.name1 = "text", .value = 43, .name2 = "asdf"};
    dump_tst(output, sut, 0);

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
    dump_tst(output, sut, 0);

    REQUIRE(output.str() == "    _type: single_nesting\n"
                            "    child:\n"
                            "        _type: single_scalar\n"
                            "        name: value\n");
}

TEST_CASE("struct with list of structs")
{
    std::stringstream output;

    std::vector<std::unique_ptr<tst_single_scalar>> child;
    child.push_back(make_node(tst_single_scalar{.name = "value1"}));
    child.push_back(make_node(tst_single_scalar{.name = "value2"}));

    tst_struct_with_struct_list sut{
        .child = std::move(child)
    };
    dump_tst(output, sut, 0);

    REQUIRE(output.str() == "    _type: struct_with_struct_list\n"
                            "    child:\n"
                            "      - _type: single_scalar\n"
                            "        name: value1\n"
                            "      - _type: single_scalar\n"
                            "        name: value2\n");
}