#include "args_parse.hpp"
#include "emitter.hpp"
#include "parser.hpp"
#include "semantics.hpp"
#include <iostream>
#include <fstream>

int main(int argc, char *argv[])
{
    auto args = parse_args(argc, argv);

    if ((args.output_path == "") && !args.dump_ast)
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


    if (std::holds_alternative<node_parse_error>(root_node.value) || !capyparser.errors.empty())
    {
        for (const auto& error : capyparser.errors)
        {
            std::cerr << args.input_path << ":" << error.error_location.line << ":" << error.error_location.column << ": " << error.error_message << "\n";
        }
        if (std::holds_alternative<node_parse_error>(root_node.value))
        {
            auto error = std::get<node_parse_error>(root_node.value);
            std::cerr << args.input_path << ":" << error.error_location.line << ":" << error.error_location.column << ": " << error.error_message << "\n";
        }
        return 1;
    }

    if (args.dump_ast) {
        dump_ast(root_node);
        return 0;
    }

    semantic_analyser analyser;
    auto error = analyser.semantic_analysis(root_node);
    if (error.has_value())
    {
        std::cerr << args.input_path << ":" << error.value().error_location.line << ":" << error.value().error_location.column << ": " << error.value().error_message << "\n";
        return 1;
    }

    std::ofstream outfile(args.output_path);
    emitter capyemitter{outfile};
    capyemitter.generate(root_node);

    outfile.close();

    return 0;
}
