# Design Decisions

This page collects design choices that influence the compiler internals and the
language model. The goal is to capture the reasoning behind decisions that are
easy to lose once the implementation evolves.

## Testing Strategy

Compiler behavior is primarily tested with Python and `pytest`. This gives the
 project a flexible way to define source-level scenarios, compiler invocations,
 and output checks.

The repository uses `uv` to simplify Python environment and test execution.

## Code Generation and Binaryen

Binaryen would be an attractive code generation backend because it can emit both
textual WAT and binary WASM and also provides optimization passes. The project
experimented with that route but postponed adoption for now.

The current blockers are practical integration issues with the existing
toolchain:

- Binaryen expects exception support, while the Clang-based WASI SDK setup used
  by the project still does not provide a clear path for enabling exceptions.
- Binaryen expects thread support, which likely requires switching the target to
  `wasm32-wasip1-threads` and then validating the rest of the toolchain.

The result is a deliberate decision to keep code generation in-project until the
toolchain constraints are better understood.

## Strings

The `string` type represents UTF-8 encoded Unicode text.

For explicit code point escapes, the language uses the flexible `\u{...}`
notation known from languages such as Rust, Swift, and PHP instead of fixed
width `\u` or `\U` escape formats.

### String Literal Storage

String literals are stored in the parse context structure together with symbol
and type information.

This keeps literals available to later compiler stages without tying them to the
AST alone. If later lowering stages operate on an IR instead of the AST, string
literal access remains straightforward and does not require duplicating that
data into multiple representations.

## Body Structure

Function bodies, and later also `if` and loop bodies, are modeled as sequences
of expressions separated by semicolons. Capylang currently treats everything as
expressions rather than mixing statements and expressions.

The current rules are:

- A body may be empty.
- A non-empty body starts with a semicolon.
- Every pair of adjacent expressions is separated by a semicolon.
- A body may end with an expression that has no trailing semicolon. In that
  case, that expression defines the body type.
- A body ending with a semicolon always has type `void`.
- `let` is modeled as an expression with type `void`.

This keeps the type model uniform and directly supports expression-oriented
constructs such as typed `if` expressions.

## Symbol Handling

The compiler uses one shared symbol concept rather than separate lookup tables
for variables, functions, and types. The concrete symbol kind is represented by
a symbol tag.

Expected advantages:

- Better diagnostics when a lookup finds a symbol of the wrong kind, such as
  calling a variable as though it were a function.
- Easier extension when new symbol kinds are introduced.

Tradeoff:

- Users of the symbol table need an additional check after lookup to confirm the
  expected symbol kind.

## Attributes

Attributes mark functions and, later, potentially other language elements. The
syntax follows a Python-like style with identifiers that start with `@`, one
attribute per line.

Simple attributes are single identifiers. More complex attributes may use a
function-like syntax with named parameters, but parameter values are restricted
to simple literals such as booleans, numbers, characters, and strings. Arbitrary
expressions are intentionally excluded to keep attribute handling simple.

Currently recognized builtin attributes:

- `@export` marks a function for export from the generated WASM module.

## Language IR Direction

The language intermediate representation, or LIR, is loosely inspired by Rust
MIR. Roughly half of the AST node shapes can be represented directly, but some
areas intentionally change in LIR.

### Explicit Load and Store Operations

LIR introduces explicit `load` and `store` expressions. These can be understood
as IR-only operations that work on a `place` abstraction rather than on
source-level expressions alone.

A `place` can represent:

- A variable
- Field access
- Pointer dereference chains

This is meant to simplify backend lowering because the emitter can distinguish
read and write contexts directly instead of reconstructing them from general AST
expressions.

### Record Initialization Lowering

Record initializers are simplified into a structure that contains:

- The record type
- The initialization expressions in field-definition order

### Type Definitions Outside LIR

Type definitions are not carried directly in LIR. Instead, LIR relies on the
symbol table as the source of truth for type information.

### Possible Future Changes

- After type inference, unresolved type variables could be removed so that LIR
  only carries fully reified types.
- Record field initializers may eventually need temporary values to preserve
  source evaluation order when field definition order differs from code order.
