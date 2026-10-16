"""Pygments lexer for Capylang."""

from pygments.lexer import RegexLexer, bygroups, words
from pygments.token import (
    Comment,
    Keyword,
    Name,
    Number,
    Operator,
    Punctuation,
    String,
    Text,
    Whitespace,
)

__all__ = ["CapylangLexer"]


_IDENTIFIER = r"[A-Za-z_$][A-Za-z0-9_$]*"
_IDENTIFIER_START = r"(?<![A-Za-z0-9_$])"
_IDENTIFIER_END = r"(?![A-Za-z0-9_$])"


class CapylangLexer(RegexLexer):
    """Highlight Capylang source code."""

    name = "Capylang"
    aliases = ["capylang", "capy"]
    filenames = ["*.capy"]
    mimetypes = ["text/x-capylang"]
    url = "https://github.com/Jokymon/capylang"

    tokens = {
        "root": [
            (r"\s+", Whitespace),
            (r"//[^\r\n]*", Comment.Single),
            (r'(b)(")', bygroups(String.Affix, String.Delimiter), "blob"),
            (r'"', String.Delimiter, "string"),
            (r"'", String.Delimiter, "character"),
            (
                rf"(@)({_IDENTIFIER})",
                bygroups(Punctuation, Name.Decorator),
            ),
            (
                rf"{_IDENTIFIER_START}(fn)(\s+)({_IDENTIFIER})",
                bygroups(Keyword.Declaration, Whitespace, Name.Function),
            ),
            (
                rf"{_IDENTIFIER_START}(record)(\s+)({_IDENTIFIER})",
                bygroups(Keyword.Declaration, Whitespace, Name.Class),
            ),
            (
                rf"{_IDENTIFIER_START}(allocate)(\s+)({_IDENTIFIER})",
                bygroups(Keyword, Whitespace, Name.Class),
            ),
            (
                rf"(:)(\s*)(\*?)({_IDENTIFIER})",
                bygroups(Punctuation, Whitespace, Operator, Keyword.Type),
            ),
            (
                rf"{_IDENTIFIER_START}(as)(\s+)(\*?)({_IDENTIFIER})",
                bygroups(Keyword, Whitespace, Operator, Keyword.Type),
            ),
            (
                rf"(->)(\s*)(\*?)({_IDENTIFIER})",
                bygroups(Operator, Whitespace, Operator, Keyword.Type),
            ),
            (
                words(
                    ("break", "else", "if", "return", "while"),
                    prefix=_IDENTIFIER_START,
                    suffix=_IDENTIFIER_END,
                ),
                Keyword.Reserved,
            ),
            (
                words(
                    ("global", "import", "include", "let"),
                    prefix=_IDENTIFIER_START,
                    suffix=_IDENTIFIER_END,
                ),
                Keyword.Declaration,
            ),
            (
                words(
                    ("allocate", "as", "fn", "mut", "record"),
                    prefix=_IDENTIFIER_START,
                    suffix=_IDENTIFIER_END,
                ),
                Keyword,
            ),
            (rf"\d+(?:{_IDENTIFIER})?{_IDENTIFIER_END}", Number.Integer),
            (rf"{_IDENTIFIER}(?=\s*::)", Name.Namespace),
            (rf"{_IDENTIFIER}(?=\s*\()", Name.Function),
            (rf"(?<=\.){_IDENTIFIER}", Name.Attribute),
            (rf"{_IDENTIFIER}", Name),
            (r"->|\|\||&&|!=|==|<=|>=|<<|>>|[-+/%*=<>]", Operator),
            (r"::|[:,;.(){}@]", Punctuation),
            (r".", Text),
        ],
        "blob": [
            (r'\\(?:[\'"]|x[0-9A-Fa-f]{2})', String.Escape),
            (r'"', String.Delimiter, "#pop"),
            (r'[^"\\]+', String),
            (r"\\.", String),
        ],
        "string": [
            (r'\\(?:[\'"]|u\{[0-9A-Fa-f]{1,6}\})', String.Escape),
            (r'"', String.Delimiter, "#pop"),
            (r'[^"\\]+', String),
            (r"\\.", String),
        ],
        "character": [
            (r'\\(?:[\'"]|u\{[0-9A-Fa-f]{1,6}\})', String.Escape),
            (r"'", String.Delimiter, "#pop"),
            (r"[^'\\]+", String.Char),
            (r"\\.", String.Char),
        ],
    }
