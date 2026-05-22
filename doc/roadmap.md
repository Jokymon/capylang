# Compiler Roadmap

This page tracks implementation work that is still open and relevant to the
compiler internals. It is intentionally more structured than a raw TODO list so
it can serve as published project documentation.

## Language and Type System

- Formalize the grammar so it can drive tooling such as syntax highlighters and
  a future language server.
- Add 64-bit integer types.
- Add floating-point types.
- Design and add vector numeric types.
- Add structured component model types such as variants, enums, lists, and
  results.
- Improve type inference for `if` and `else` branches when surrounding type
  information is available.
- Add the unary logical `!` operator.
- The type inference doesn't properly pick up on the return type when only
  `return` statements are used.

## Memory and Data Model

- Extend the basic memory management model.
- Tighten pointer dereference handling, especially for `u8` and `u16` where the
  current defaults differ from normal `u32` dereferencing.
- Design the `address of` operator and define where it should be legal.
- Decide whether global initialization should allow the `const` subset of
  instructions.
- Explore a feature for embedding binary assets directly from files.
- Clarify the in-memory lowering strategy for strings and WASI interop.

## Frontend and Diagnostics

- Fix parser cases that currently allow endless loops on unknown or unexpected
  tokens.
- Improve lexer-side error handling and diagnostics.
- Decide whether `let` expressions without a trailing semicolon should stay
  valid.

## Modules, ABI, and Runtime Surface

- Introduce a concept of symbol visibility.
- Turn explicit import structures for functions into regular attributes.
- Add support for multiple WebAssembly memories and define whether that should
  be explicit in the language.
- Design a library, module, or package concept for source reuse and module
  mapping.
- The `unreachable()` intrinsic allows for some very simple abort/fail semantic,
  however the stack unwinding/traceback in this scenario is still very limited
  and only shows the function names. Having more detailed information here would
  be nice, including the line number in the capy source, the source file name
  and depending on what DWARF/the wasm runtime can provide, maybe even more
  information, like function signature.

## Intrinsics

- Add core numeric intrinsics such as `abs`, `sqrt`, `ceil`, `floor`, `trunc`,
  `min`, and `max`.
- Add memory-related intrinsics such as `size`, `grow`, `copy`, and `fill`.
- Decide whether compiler intrinsics should be declared through something like
  an `@intrinsic` attribute.

## AST Refactoring

The long-term direction is to move toward dedicated AST node types wherever that
is practical. The motivation is to reduce variant checks and make the compiler
internals easier to follow.

Current status:

1. Parsing errors were converted to support multi-error handling.
2. Non-expression nodes have already moved away from the generic `ast_node`
   representation.

Remaining work:

1. Continue replacing generic AST returns with more specific node types where
   possible.
2. Remove the remaining `located`-style wrapping from AST nodes, similar to what
   was already done for tokens.

## Design Constraints Learned Elsewhere

These points are not project goals, but they document pitfalls the language
design should avoid:

- Pattern matching on sum types should avoid the kind of awkwardness associated
  with `std::visit`.
- Pointer handling should avoid unnecessary verbosity.
- Missing returns in functions should be treated more strictly than in C++.
