#include "../args_parse.hpp"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("parse_args reads input/output options", "[args]")
{
    char arg0[] = "capylang";
    char arg1[] = "-i";
    char arg2[] = "example.capy";
    char arg3[] = "-o";
    char arg4[] = "example.wasm";
    char *argv[] = {arg0, arg1, arg2, arg3, arg4};

    const Args parsed = parse_args(5, argv);

    REQUIRE(parsed.input_path == "example.capy");
    REQUIRE(parsed.output_path == "example.wasm");
    REQUIRE(parsed.dump_ast == false);
}

TEST_CASE("parse_args enables dump-ast flag", "[args]")
{
    char arg0[] = "capylang";
    char arg1[] = "--dump-ast";
    char *argv[] = {arg0, arg1};

    const Args parsed = parse_args(2, argv);

    REQUIRE(parsed.dump_ast);
}
