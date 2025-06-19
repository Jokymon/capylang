#include "args_parse.hpp"
#include "emitter.hpp"
#include "parser.hpp"
#include <cctype>
#include <iostream>
#include <fstream>
#include <string>

int main(int argc, char* argv[]) {
    auto args = parse_args(argc, argv);

    if (args.output_path == "") {
        std::cerr << "Argument required: -o\n";
        return 1;
    }
    if (args.input_path == "") {
        std::cerr << "Argument required: -i\n";
        return 1;
    }

    std::ifstream infile(args.input_path);
    lexer capylexer{infile};
    parser capyparser{capylexer};
    auto root_node = capyparser.parse();

    std::ofstream outfile(args.output_path);
    emitter capyemitter{outfile};

    int exit_code = std::visit([&](const auto& ast) -> int {
        using T = std::decay_t<decltype(ast)>;
        if constexpr (std::is_same_v<T, node_number>) {
            capyemitter.emit(ast);
            return 0;
        } else if constexpr (std::is_same_v<T, node_parse_error>) {
            std::cerr << "Compile error: " << ast.error_message << "\n";
            return 1;
        }
    }, root_node);

    outfile.close();

    return exit_code;
}
