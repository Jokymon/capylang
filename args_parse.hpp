#include <string>

struct Args
{
    std::string input_path;
    std::string output_path;
};

Args parse_args(int argc, char *argv[]);