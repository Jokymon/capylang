#include <string>

struct Args
{
    std::string input_path;
    std::string output_path;
    bool dump_ast;
};

Args parse_args(int argc, char* argv[]);