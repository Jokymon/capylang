#pragma once
#include <string>

struct Args
{
    std::string programm_name;

    std::string input_path;
    std::string output_path;
    bool dump_tokens = false;
    bool dump_ast = false;
    bool dump_anf = false;

    bool help = false;
};

std::string generate_help_text(Args const& args);
Args parse_args(int argc, char* argv[]);
