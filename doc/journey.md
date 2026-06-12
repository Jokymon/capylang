# The journey so far

## Handcrafting the lexer and parser

With the goal of producing programmer-friendly, helpful, specific, and clear
compiler errors and warnings, I followed the common idea that this would
require a handcrafted parser. My thinking was that a simple top-down parser,
combined with an equally simple handcrafted lexer, should not be too difficult
to build and would give me the freedom to produce highly situation- and
location-dependent error messages with enough context to actually help.

Work on this moved forward quickly. With a clear naming convention for the
parsing functions, it felt like an approach that would let me keep a good
overview of the code. As I added more language features, including control
structures like `if` and `while`, and even function attributes with prepended
`@` elements, it became clear that this approach could still work well for
quite some time. Adding new grammar features remained easy. The only slowdown
came from writing all the unit tests and failing code examples, together with the
expected diagnostics for the failing parse cases.

One advantage I clearly see in this approach is the independence from external
tooling, which may require additional languages or environments. ANTLR, for
example, would require Java. Lex/yacc would require extra executables, and
other tooling might again depend on different executables or even whole
language ecosystems. Still, since Python is already one of the major
dependencies of this project, that may not be a particularly strong argument.

Once I started thinking about building and maintaining tooling around Capylang,
the need for a formally defined grammar became more obvious though. To build
tools such as language servers, syntax highlighters, and linters, having a
reference grammar is an important foundation. That was the point where I started deriving
an ANTLR grammar from the handwritten parser. So far, though, I have decided to
focus on keeping that grammar in sync with my own implementation of the
language by automating some form of consistency check. The existing compiler
has already come quite far, and I did not want to replace the entire frontend
just to introduce different tooling at this stage.

Over time, more formal descriptions may find their way into the compiler, for
example for the AST, to make further development easier and more ergonomic. So
the idea of replacing the handwritten parser with an ANTLR-based
implementation, or with some other tooling and grammar, is not completely off
the table. In the short term, though, the main compiler will remain
handwritten, while the ANTLR part may find its place in some of the surrounding
tooling. One area where this has already happened is the automatic updating of
the keywords section in the TextMate grammar used for VSCode highlighting.
