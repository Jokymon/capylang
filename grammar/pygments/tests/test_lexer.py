from pygments import lex
from pygments.lexers import get_lexer_by_name, get_lexer_for_filename
from pygments.token import Comment, Keyword, Name, Number, Operator, String

from pygments_capylang import CapylangLexer


def token_pairs(source: str):
    return list(lex(source, CapylangLexer(ensurenl=False)))


def test_plugin_is_discoverable():
    assert isinstance(get_lexer_by_name("capylang"), CapylangLexer)
    assert isinstance(get_lexer_by_name("capy"), CapylangLexer)
    assert isinstance(get_lexer_for_filename("example.capy"), CapylangLexer)


def test_declarations_types_and_literals():
    tokens = token_pairs(
        "@export\n"
        "fn answer(value: u32) -> u32 {\n"
        "  // The answer\n"
        "  let byte: *u8 = 42u32;\n"
        "  let text = \"capy \\u{1f600}\";\n"
        "  let data = b\"capy\\x21\";\n"
        "}\n"
    )

    assert (Name.Decorator, "export") in tokens
    assert (Keyword.Declaration, "fn") in tokens
    assert (Name.Function, "answer") in tokens
    assert (Keyword.Type, "u32") in tokens
    assert (Operator, "*") in tokens
    assert (Number.Integer, "42u32") in tokens
    assert (String.Escape, r"\u{1f600}") in tokens
    assert (String.Affix, "b") in tokens
    assert (String.Escape, r"\x21") in tokens
    assert (Comment.Single, "// The answer") in tokens


def test_keywords_do_not_match_inside_identifiers():
    tokens = token_pairs("let $if_value = gift(); fn compact()->u32 { 1u32 }")

    assert (Keyword.Declaration, "let") in tokens
    assert (Name, "$if_value") in tokens
    assert (Name.Function, "gift") in tokens
    assert (Name.Function, "compact") in tokens
    assert (Keyword.Type, "u32") in tokens
    assert (Keyword.Reserved, "if") not in tokens
