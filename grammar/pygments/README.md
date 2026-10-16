# Pygments lexer for Capylang

This package adds Capylang syntax highlighting to
[Pygments](https://pygments.org/) without requiring the lexer to be included in
Pygments itself. It follows the tokens formalized in the ANTLR grammar in the
parent directory and registers itself through Pygments' plugin entry point.

## Install

### Workflow package

Download the `pygments-capylang-<commit>` artifact from a successful
`Package Pygments plugin` GitHub Actions run. The artifact contains a wheel and
a source archive. Install the wheel directly:

```sh
python -m pip install pygments_capylang-*.whl
```

### Source checkout

From a Capylang source checkout:

```sh
python -m pip install ./grammar/pygments
```

Once installed, any application that discovers Pygments plugins can select the
lexer by the `capylang` or `capy` alias, by the `*.capy` filename, or by the
`text/x-capylang` MIME type. For example:

```sh
pygmentize -l capylang -f terminal256 examples/example.capy
pygmentize -l capylang -f html -O full -o example.html examples/example.capy
```

This also makes `capylang` available as a fenced-code language to documentation
tools that use Pygments plugin discovery, such as Sphinx and MkDocs:

````markdown
```capylang
fn answer() -> u32 { 42u32 }
```
````

## Development

Install the package in editable mode with its test dependency, then run pytest:

```sh
python -m pip install -e "./grammar/pygments[test]"
python -m pytest grammar/pygments/tests
```
