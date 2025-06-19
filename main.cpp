#include "args_parse.hpp"
#include "parser.hpp"
#include <cctype>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

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

    std::ofstream outfile(args.output_path);
    std::ifstream infile(args.input_path);

    lexer capylexer{infile};
    parser capyparser{capylexer};

    auto root_node = capyparser.parse();

    int exit_code = std::visit([&](const auto& ast) -> int {
        using T = std::decay_t<decltype(ast)>;
        if constexpr (std::is_same_v<T, node_number>) {
            outfile << "(module\n";
            outfile << "  (type (;0;) (func))\n";
            outfile << "  (type (;1;) (func (param i32)))\n";
            outfile << "  (import \"wasi_snapshot_preview1\" \"proc_exit\" (func $__imported_wasi_snapshot_preview1_proc_exit (;0;) (type 1)))\n";
            outfile << "  (memory (;0;) 2)\n";
            outfile << "  (export \"memory\" (memory 0))\n";
            outfile << "  (export \"_start\" (func $_start))\n";
            outfile << "  (func $_start (type 0)\n";
            outfile << "      i32.const " << ast.number << "\n";
            outfile << "      call $__imported_wasi_snapshot_preview1_proc_exit\n";
            outfile << "      unreachable\n";
            outfile << "  )\n";
            outfile << ")\n";

            return 0;
        } else if constexpr (std::is_same_v<T, node_parse_error>) {
            std::cerr << "Compile error: " << ast.error_message << "\n";
            return 1;
        }
    }, root_node);

    outfile.close();

    return exit_code;
}
