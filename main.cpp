#include "args_parse.hpp"
#include "emitter.hpp"
#include "parser.hpp"
#include <iostream>
#include <fstream>

int main(int argc, char *argv[])
{
    auto args = parse_args(argc, argv);

    if (args.output_path == "")
    {
        std::cerr << "Argument required: -o\n";
        return 1;
    }
    if (args.input_path == "")
    {
        std::cerr << "Argument required: -i\n";
        return 1;
    }

    std::ifstream infile(args.input_path);
    lexer capylexer{infile};
    parser capyparser{capylexer};
    auto root_node = capyparser.parse();

    std::ofstream outfile(args.output_path);
    emitter capyemitter{outfile};

    if (std::holds_alternative<node_parse_error>(root_node))
    {
        std::cerr << "Compile error: " << std::get<node_parse_error>(root_node).error_message << "\n";
        return 1;
    }
    capyemitter.emit(root_node);

    outfile.close();

    return 0;
}
