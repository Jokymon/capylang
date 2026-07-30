# Compiler Roadmap

This page tracks implementation work that is still open and relevant to the
compiler internals.

## Language and Type System

- Add 64-bit integer types.
- Add floating-point types.
- Add `\0` shorthand for inserting a zero-byte into a string
- Check type inference when accessing ptr/size of a string whose type was only inferred
- Design and add vector numeric types.
- Add structured component model types such as variants, enums, lists, and
  results.
- Improve type inference for `if` and `else` branches when surrounding type
  information is available.
- Add logical `&`, `|` operators
- Add the unary logical `!` operator.
- The type inference doesn't properly pick up on the return type when only
  `return` statements are used.
- Add compile time constants, for starters, at least the numeric ones.

## Memory and Data Model

- Extend the basic memory management model.
- Consider some form of memory configuration such that we can later define the area for heap allocations
  and for placing data sections. This might either be in the form of more command line arguments or
  something like linker scripts or just configurations.
- Tighten pointer dereference handling, especially for `u8` and `u16` where the
  current defaults differ from normal `u32` dereferencing.
- Design the `address of` operator and define where it should be legal.
- Explore a feature for embedding binary assets directly from files.
- Clarify the in-memory lowering strategy for strings and WASI interop.
- Add better checks for the supported range of types and functions in global initialisations

## Frontend and Diagnostics

- Investigate the current weaknesses of the hand-written parser. In some cases, it just ends up in an endless loop,
  sometimes it skips over the rest of the file complaining that there was some unexpected trailer.
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
