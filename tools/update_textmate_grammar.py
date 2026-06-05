import json
import pathlib
import re
import sys
from collections import OrderedDict


REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent
LEXER_PATH = REPO_ROOT / "grammar" / "CapylangLexerGrammar.g4"
SCOPE_MAP_PATH = REPO_ROOT / "grammar" / "textmate_keyword_scopes.json"
TEXTMATE_PATH = REPO_ROOT / "capylang" / "syntaxes" / "capy.tmLanguage.json"

TOKEN_RULE_RE = re.compile(
    r"^(?P<token>[A-Z_][A-Z0-9_]*)\s*:\s*'(?P<literal>[^']+)'\s*;\s*$"
)


def load_scope_map(path: pathlib.Path) -> dict[str, str]:
    with path.open(encoding="utf-8") as handle:
        data = json.load(handle)

    if not isinstance(data, dict):
        raise ValueError(f"{path} must contain a JSON object")

    scope_map: dict[str, str] = {}
    for token_name, scope in data.items():
        if not isinstance(token_name, str) or not isinstance(scope, str):
            raise ValueError(f"{path} entries must map strings to strings")
        scope_map[token_name] = scope
    return scope_map


def extract_keyword_tokens(
    lexer_path: pathlib.Path, scope_map: dict[str, str]
) -> OrderedDict[str, list[str]]:
    grouped_literals: OrderedDict[str, list[str]] = OrderedDict()

    with lexer_path.open(encoding="utf-8") as handle:
        for raw_line in handle:
            line = raw_line.strip()
            match = TOKEN_RULE_RE.match(line)
            if match is None:
                continue

            token_name = match.group("token")
            if token_name not in scope_map:
                continue

            scope_name = scope_map[token_name]
            literal = match.group("literal")
            grouped_literals.setdefault(scope_name, []).append(literal)

    return grouped_literals


def build_keyword_patterns(grouped_literals: OrderedDict[str, list[str]]) -> list[dict[str, str]]:
    patterns = []
    for scope_name, literals in grouped_literals.items():
        escaped_literals = [re.escape(literal) for literal in literals]
        pattern = r"\b(" + "|".join(escaped_literals) + r")\b"
        patterns.append(
            {
                "name": scope_name,
                "match": pattern,
            }
        )
    return patterns


def update_textmate_grammar(
    textmate_path: pathlib.Path, keyword_patterns: list[dict[str, str]]
) -> None:
    with textmate_path.open(encoding="utf-8") as handle:
        grammar = json.load(handle)

    grammar["repository"]["keywords"]["patterns"] = keyword_patterns

    with textmate_path.open("w", encoding="utf-8", newline="\n") as handle:
        json.dump(grammar, handle, indent="\t", ensure_ascii=False)
        handle.write("\n")


def main() -> int:
    scope_map = load_scope_map(SCOPE_MAP_PATH)
    grouped_literals = extract_keyword_tokens(LEXER_PATH, scope_map)
    keyword_patterns = build_keyword_patterns(grouped_literals)
    update_textmate_grammar(TEXTMATE_PATH, keyword_patterns)

    literal_count = sum(len(literals) for literals in grouped_literals.values())
    print(
        f"Updated {TEXTMATE_PATH} with {literal_count} keyword literals "
        f"across {len(keyword_patterns)} TextMate scopes."
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
