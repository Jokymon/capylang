#include "anf.hpp"
#include "args_parse.hpp"
#include "diagnostics.hpp"
#include "emitter.hpp"
#include "normalization.hpp"
#include "parser.hpp"
#include "semantics.hpp"
#include "type_inference.hpp"
#include "wasm_generator.hpp"
#include "wat_generator.hpp"
#include <iostream>
#include <fstream>

int main(int argc, char* argv[])
{
    auto args = parse_args(argc, argv);

    if (args.help)
    {
        std::cout << generate_help_text(args);
        return 0;
    }

    if ((args.output_path == "") && !args.dump_ast && !args.dump_anf)
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
    if (infile.fail())
    {
        std::cerr << "Couldn't open input file '" << args.input_path << "': " << strerror(errno) << "\n";
        return 1;
    }

    std::shared_ptr<lexer> capylexer = std::make_shared<lexer>(infile, args.input_path);
    context parse_context;
    parser capyparser{capylexer, parse_context};
    auto root_node = capyparser.parse();

    if (capyparser.diagnostics().has_errors())
    {
        print_diagnostics(std::cerr, capyparser.diagnostics());
        return 1;
    }

    type_inference inference{parse_context};
    inference.infer_types(root_node);
    if (inference.diagnostics().has_errors())
    {
        print_diagnostics(std::cerr, inference.diagnostics());
        return 1;
    }

    semantic_analyser analyser{parse_context};
    analyser.semantic_analysis(root_node);
    if (analyser.diagnostics().has_errors())
    {
        print_diagnostics(std::cerr, analyser.diagnostics());
        return 1;
    }

    if (args.dump_ast)
    {
        dump_module(parse_context, root_node);
        return 0;
    }

    normalization normalizer{parse_context};
    normalizer.normalize(root_node);

    if (args.dump_anf)
    {
        anf_generator anf_gen{parse_context};
        auto anf_module = anf_gen.generate(root_node);
        // if (anf_gen.diagnostics().has_errors())
        // {
        //     print_diagnostics(std::cerr, anf_gen.diagnostics());
        //     return 1;
        // }
        dump_anf_module(parse_context, anf_module);
        return 0;
    }

    emitter capyemitter{parse_context};
    wasm_module mod = capyemitter.generate(root_node);

    if (args.output_path.ends_with(".wasm"))
    {
        wasm_generator generator;

        std::ofstream outfile(args.output_path, std::ios::out | std::ios::binary);
        generator.generate(mod, outfile);
        outfile.close();
    }
    else
    {
        wat_generator generator;

        std::ofstream outfile(args.output_path);
        generator.generate(mod, outfile);
        outfile.close();
    }

    return 0;
}
