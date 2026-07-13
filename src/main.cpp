#include "args_parse.hpp"
#include "diagnostics.hpp"
#include "lir_based_emitter.hpp"
#include "lir_dump.hpp"
#include "lir_generator.hpp"
#include "lir_lowering.hpp"
#include "normalization.hpp"
#include "parser.hpp"
#include "semantics.hpp"
#include "type_inference.hpp"
#include "wasm_generator.hpp"
#include "wat_generator.hpp"
#include <iostream>
#include <fstream>

void dump_value(std::ostream& out, context& context, const source_range& range)
{
    out << range.start.filename << ":" << range.start.line << ":" << range.start.column;
}

void dump_value(std::ostream& out, context& context, const function_signature& sig)
{
    out << context.repr(sig.function_type);
}

void dump_value(std::ostream& out, context& context, const type_id& value)
{
    out << context.repr(value);
}

void dump_value(std::ostream& out, context& context, const symbol_id& value)
{
    out << context.repr(value);
}

int main(int argc, char* argv[])
{
    auto args = parse_args(argc, argv);

    if (args.help)
    {
        std::cout << generate_help_text(args);
        return 0;
    }

    if ((args.output_path == "") && !args.dump_ast && !args.dump_lir)
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

    diagnostic_bag diagnostics;
    std::shared_ptr<lexer> capylexer = std::make_shared<lexer>(diagnostics, infile, args.input_path);

    if (args.dump_tokens)
    {
        while (!capylexer->ahead_is<token_eof>())
        {
            const token& tok = capylexer->peek_token();
            std::cout << std::visit(
                [](const auto& n) -> std::string
                {
                              using T = std::decay_t<decltype(n)>;

                              if constexpr (std::is_same_v<T, token_eof>) {
                                return "<TOK_EOF>";
                              } else if constexpr (std::is_same_v<T, token_identifier>) {
                                return "<TOK_ID: "+n.name+">";
                              } else if constexpr (std::is_same_v<T, token_illegal>) {
                                return "<TOK_ILLEGAL: "+n.token_text+">";
                              } else if constexpr (std::is_same_v<T, token_integer>) {
                                return "<TOK_INT: "+std::to_string(n.number)+">";
                              } else if constexpr (std::is_same_v<T, token_char_literal>) {
                                return "<TOK_CHAR: "+std::to_string(n.ch)+">";
                              } else if constexpr (std::is_same_v<T, token_string_literal>) {
                                return "<TOK_STR: "+n.str+">";
                              } else if constexpr (std::is_same_v<T, token_symbol>) {
                                return "<TOK_SYM: "+n.to_string()+">";
                              } else {
                                return "<!TOK_FAIL!>";
                              } },
                tok
            );
            std::cout << " ";
            capylexer->next_token();
        }
        std::cout << "\n";
        return 0;
    }

    context parse_context;
    parser capyparser{diagnostics, capylexer, parse_context};
    auto root_node = capyparser.parse();

    if (diagnostics.has_errors())
    {
        print_diagnostics(std::cerr, diagnostics);
        return 1;
    }

    type_inference inference{diagnostics, parse_context};
    inference.infer_types(root_node);
    if (diagnostics.has_errors())
    {
        print_diagnostics(std::cerr, diagnostics);
        return 1;
    }

    semantic_analyser analyser{diagnostics, parse_context};
    analyser.semantic_analysis(root_node);
    if (diagnostics.has_errors())
    {
        print_diagnostics(std::cerr, diagnostics);
        return 1;
    }

    if (args.dump_ast)
    {
        dump_node(std::cout, parse_context, root_node);
        return 0;
    }

    normalization normalizer{parse_context};
    normalizer.normalize(root_node);

    lir_generator lir_gen{parse_context};
    auto l_mod = lir_gen.generate(root_node);
    lir_lowering l_lower{parse_context};
    l_lower.visit(l_mod);

    if (args.dump_lir)
    {
        lir::dump(std::cout, parse_context, l_mod);
        return 0;
    }

    lir_based_emitter l_emitter{parse_context};
    wasm_module mod = l_emitter.generate(l_mod);

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
