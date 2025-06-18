#include <iostream>
#include <fstream>
#include <string>
#include <vector>

struct Args {
    std::string output_path;
};

Args parse_args(int argc, char* argv[]) {
    Args arguments;

    for (int i=0; i<argc; i++) {
        std::string arg{argv[i]};
        if (arg.starts_with("-o")) {
            if (arg.size()==2) {
                if (i+1<argc) {
                    // we have to take the argument for -o from the next argv
                    // entry
                    arguments.output_path = std::string{argv[++i]};
                }
            } else {
                // -o and the argument come in one and we just need to remove
                // the argument name
                arguments.output_path = arg.substr(2);
            }
        }
    }
    return arguments;
}

int main(int argc, char* argv[]) {
    auto args = parse_args(argc, argv);

    if (args.output_path == "") {
        std::cerr << "Argument required: -o\n";
        return 1;
    }

    std::ofstream outfile(args.output_path);

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
