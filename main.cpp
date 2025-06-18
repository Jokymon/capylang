#include <iostream>
#include <fstream>
#include <string>
#include <vector>

std::vector<std::string> parse_args(int argc, char* argv[]) {
    std::vector<std::string> arguments;

    for (int i=0; i<argc; i++) {
        arguments.emplace_back(std::string{argv[i]});
    }
    return arguments;
}

int main(int argc, char* argv[]) {
    auto args = parse_args(argc, argv);

    if (args.size()<2) {
        std::cerr << "One argument required: output file path\n";
        return 1;
    }

    std::ofstream outfile(args[1]);

    outfile << "(module\n";
    outfile << "  (type (;0;) (func))\n";
    outfile << "  (type (;1;) (func (param i32)))\n";
    outfile << "  (import \"wasi_snapshot_preview1\" \"proc_exit\" (func $__imported_wasi_snapshot_preview1_proc_exit (;0;) (type 1)))\n";
    outfile << "  (memory (;0;) 2)\n";
    outfile << "  (export \"memory\" (memory 0))\n";
    outfile << "  (export \"_start\" (func $_start))\n";
    outfile << "  (func $_start (type 0)\n";
    outfile << "      i32.const 3\n";
    outfile << "      call $__imported_wasi_snapshot_preview1_proc_exit\n";
    outfile << "      unreachable\n";
    outfile << "  )\n";
    outfile << ")\n";
    outfile.close();

    return 0;
}
