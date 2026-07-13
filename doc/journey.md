# The journey so far

## Major Milestones

 * **17.06.2025** project initialisation
 * **17.09.2025** adding dump functions for AST
 * **08.10.2025** First version of a VSCode language support extension
 * **02.12.2025** Introduce WASM IR to code generation
 * **27.02.2026** explicit AST-visitor (used for type inference, later also semantic checks)
 * **03.03.2026** introduction of ANF structure
 * **07.05.2026** adding of X-Macros
 * **21.05.2026** initial LIR, replacing ANF approach
 * **22.05.2026** Add GitHub CI automation
 * **13.07.2026** Big move to LIR

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

## Dealing with data structures

One of the core elements of any compiler is probably the abstract syntax tree
(AST). So even with my limited compiler experience so far, it is no surprise
that the first hints of an AST appeared as early as three days after development
started, in the 11th commit.
At that point, the AST was represented only as hand-written C++ structs. That
approach was used, adapted, and extended for quite a long time. I even added
functions to dump an AST, and later a hand-written visitor base class, before I
started looking for a way to separate the AST structure from the C++
implementation. After introducing an intermediate representation for the WASM
stage, and after thinking about another intermediate representation between the
AST and the WASM IR, I became increasingly convinced that these data structures
should be represented more independently from their implementation. Keeping the
data, the dump functions, and the visitor in sync became more and more tedious.
Introducing even more data structures in the same style would only have made
this more annoying.
My first thought was to keep all of this purely within C++ and let the C++
compiler generate the surrounding tooling. With C++26 and the introduction of
compile-time reflection, I thought this should become relatively easy. Sadly, at
the time of writing, even the newest Clang version still does not officially
support this feature, at least not in the Clang WASM-SDK. So I did the only
other thing I knew that somewhat allows this kind of separation: I introduced
X-Macros.
Migrating the definitions, dump functions, and visitor was fairly easy, and
within only a few commits I had fully converted the AST to an X-Macro-based
setup. Later, when I started thinking about an intermediate representation, I
could already rely on an established infrastructure for defining my IR nodes.
Over time, however, I started to feel increasingly bogged down by the complexity
of these X-Macros. They generate code behind the scenes, which is not easy to
see and can make debugging more difficult. The problems were never huge, and
they did not cause any major headaches, but once I started thinking about an
automatically generated transforming visitor, I felt that the X-Macros would
become too hard to maintain. I also started thinking about describing
transformations within a tree structure, or from one tree structure to another,
in a more declarative style. That is where the X-Macro approach would definitely
fall short. This is when I started developing
[Silwright](https://pypi.org/project/silwright/). The goal of this project is to
describe node structures, and perhaps later even transformations, in a simple
DSL, then generate the corresponding C++ structs, visitors, dump functions, and
now also transformers from those descriptions.
At the time of writing this journey entry, I have reached a workable version
0.2.0 of Silwright, and I am in the middle of migrating again. My hope is that
this change will be more sustainable in the long run.
