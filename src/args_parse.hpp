#pragma once
#include "compile_options.hpp"
#include <optional>
#include <string>

struct Args
{
    std::string programm_name;

    std::string input_path;
    std::string output_path;

    memory_type mem_type = memory_type::exported;
    size_t mem_min_blocks = 1;
    std::optional<size_t> mem_max_blocks = std::nullopt;

    bool dump_tokens = false;
    bool dump_ast = false;
    bool dump_lir = false;

    bool help = false;
};

std::string generate_help_text(Args const& args);
Args parse_args(int argc, char* argv[]);
