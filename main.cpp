#include "args_parse.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

struct Args {
    std::string input_path;
    std::string output_path;
};

void parse_option(const char* option, ArgvIterator& start, const ArgvIterator &end, std::string& output_string) {
    if ((*start).starts_with(option)) {
        if ((*start).size()==strlen(option)) {
            // we have to take the argument for the given option from the next
            // argument
            if (++start != end) {
                output_string = *start;
            }
        } else {
            // The option is already included in the argument and has to be
            // removed
            output_string = (*start).substr(strlen(option));
        }
    }
}

Args parse_args(int argc, char* argv[]) {
    Args arguments;

    ArgvRange args{argc, argv};
    auto args_start = args.begin();
    auto args_end = args.end();

    for (; args_start != args_end; args_start++) {
        parse_option("-o", args_start, args_end, arguments.output_path);
        parse_option("-i", args_start, args_end, arguments.input_path);
    }
    return arguments;
}

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
