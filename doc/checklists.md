# Checklists

Make sure to check the following lists according to the modifications you made to the compiler.

# When changing the grammar in general

( ) Make sure to have Python-based unit tests checking the good case of the language as well as some tests checking
    parsing errors and the reported messages.
( ) Update the ANTLR grammars for parser `grammar/CapylangParserGrammar.g4` and lexer `grammar/CapylangLexerGrammar.g4`.
    If you don't update these files, you should get a fail from the test in `tests/test_tooling.py`.
( ) Consider what design decisions you had to make for this change and add a corresponding entry to
    `doc/design-decisions.md`.

# When adding a new keyword to the grammar

( ) Check all items under "When changing the grammar in general"
( ) Add mappings from the keyword entries to a Textmate scope in `grammar/textmate_keyword_scopes.json`
( ) Run `uv run ./tools/update_textmate_grammar.py` to update the Textmate grammar