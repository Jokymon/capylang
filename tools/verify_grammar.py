import antlr4_tool_runner
import glob
import subprocess
import sys
from antlr4 import *


def antlr4_cli_wrapper():
#def run_cli(entrypoint):
    antlr4_tool_runner.initialize_paths()
    args, version = antlr4_tool_runner.process_args()
    jar, java = antlr4_tool_runner.install_jre_and_antlr(version)

    cp = subprocess.run([java, '-cp', jar, "org.antlr.v4.Tool"] + args)
    return cp.returncode


def generate_parser_files():
    # Overwrite sys.argv for the antlr4 toolrunning
    old_sys_argv = sys.argv
    sys.argv = [ sys.argv[0], "-Dlanguage=Python3", "./grammar/CapylangParserGrammar.g4", "./grammar/CapylangLexerGrammar.g4", "-o", "tools" ]
    cp = antlr4_cli_wrapper()
    sys.argv = old_sys_argv
    return cp


def parse_file(file_path) -> bool:
    from CapylangLexerGrammar import CapylangLexerGrammar
    from CapylangParserGrammar import CapylangParserGrammar

    input_stream = FileStream(file_path)
    lexer = CapylangLexerGrammar(input_stream)
    stream = CommonTokenStream(lexer)
    parser = CapylangParserGrammar(stream)
    # lexer.removeErrorListeners()
    # parser.removeErrorListeners()
    print("Checking ", file_path, " ", end='')
    tree = parser.module()

    error_count = parser.getNumberOfSyntaxErrors()
    if error_count>0:
        print("FAILED")
        return False
    else:
        print("PASSED")
        return True


if __name__ == "__main__":
    if generate_parser_files()!=0:
        print("Grammar generation failed!")
    else:
        file_count = 0
        pass_count = 0
        for path in glob.glob("./corpus/pass/*.capy"):
            file_count += 1
            if parse_file(path):
                pass_count += 1

        print(f"{pass_count} of {file_count} pass parsing")