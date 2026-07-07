import pytest
import sys
sys.path.append("./tools")
sys.path.append("./tools/grammar")  # for the Linux version :-(
import extract_samples
import verify_grammar


@pytest.mark.tooling
@pytest.mark.no_compile_matrix
def test_antlr_grammar_accepts_passing_test_snippets():
    extract_samples.main()

    cp = verify_grammar.generate_parser_files()
    assert cp==0, "Generating the parser tooling failed"

    (pass_count, file_count) = verify_grammar.parse_all_corpus(verbose=False)
    assert pass_count == file_count
