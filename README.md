## Goals

 * general purpose programming language that only targets WASM
   --> it may use WASM (WASM component model) specific concepts
 * build a WASM compiler that runs in a WASM runtime with WASI p1
   or maybe p2

## Testing

A simple framework was set in place to run compiled WASM modules and check their output and
exit codes. This is supported by `uv` and the `pytest` library.

## TODO
 * formalizing the grammar
   --> derive syntax highlighters and a language server
 * document or even automate the setup of the development tools
   --> WASI SDK + Sysroot
   --> Wasm tools
   --> Wasmtime or other WASM runtimes

 * add other basic number types (f32, f64, s64, s16, s8, ...)
 * add structured component model types (variant, list, record, result, ...)
 * implement a simple heap management concept (check the WAMR/Emscripten memory organisation)
 * Add structured types (struct, list, enum/unions)
 * Add some form of memory management
 * Add intrinsics
 * Make sure pointer types are correctly dereferenced; special checks for u8, u16, since we dereference u32 normally
 * Comparison operators and bool type

### Big refactoring

The final goal is to use dedicated AST node types (if possible). If we manage to do that, then a lot of code should get
simpler, because we no longer need to check for th variant of AST node in a lot of places.

This refactoring however has multiple steps:

 1. All the parsing errors need to be converted to "multi-error" capable errors. This is necessary because so far,
    still a few of the parsing functions can still either return a valid node or an error node. But we need a valid
    though potentially dummy value for all the nodes.
    --> COMPLETED
 1. Step by step replace the generic AST node return by more specific returns. Some returns can be very specific, for
    example a `node_module` can only ever be a `node_module`, but for expressions, we could still have some generic
    parent type. This is where we would keep things such as a node type which is not needed for other AST nodes.
 1. Finally or maybe partially interlinked with the previous step, we can get rid of the `located` concept for AST nodes
    as we did with the tokens. This should make the type definitions and the code for accessing the AST node fields
    much easier.

### String implementation

The idea for strings and interop with WASI (preview1 and component model) is that strings would implicitly represent a
record with the fields ptr: *u32 and size: u32 and that these fields are implicitly made available by the compiler.

This also raises the question, how exactly such a structure would be represented in memory. We could start thinking
about different lowering strategies and for example store a string in two local variables in some cases.

### Inspiration for language design:

 - Discussion about dereference operator prefix/postfix: https://www.reddit.com/r/Compilers/comments/1bl7c1m/why_is_the_dereference_operator_generally_a/
 - The Nox language: https://codeberg.org/nox-language/nox
 - Things hated about Rust:
   * part 1: https://blog.yossarian.net/2020/05/20/Things-I-hate-about-rust
   * part 2: https://blog.yossarian.net/2022/03/10/Things-I-hate-about-Rust-redux
- Thoughts about the future of RAII: https://verdagon.dev/blog/raii-next-steps
- You need subtyping: https://blog.polybdenum.com/2025/03/26/why-you-need-subtyping.html
- Safe navigation operator: https://en.wikipedia.org/wiki/Safe_navigation_operator
- Null coalescing operator: https://en.wikipedia.org/wiki/Null_coalescing_operator
- Chocopy WASM backend: https://yangdanny97.github.io/blog/2022/10/11/chocopy-wasm-backend

## Decisions

### Using Binaryen as code generator

Binaryen would be a good generator to easily produce WASM WAT format but also binary WASM modules. Additionally it would
even include some optimisation steps out of the box. However during first experiments of adding the project as CPM
packet, several problems were encountered and the idea was ultimately abandoned for the moment.

 * Binaryen needs support for exceptions; We currently use the Clang-based WASI SDK which as of 23.10.2025 still claims
   to not support exceptions. The option `-fno-exception` could be removed from `CMakeLists.txt` but it is unclear
   whether this really works. Some compiler errors still showed up.
 * Binaryen needs threads support; With a simple switch to target `wasm32-wasip1-threads` this should be fixable but
   also here might be more problems than initially seen.

### Encoding Unicode code points

To insert specific unicode code points in strings, we use the more flexible notation `\u{xxx}` as used in Rust, Swift or
PHP rather than the stricter, fixed length variants with `\u` and `\U` in Java, C, C++ or C#.

### Structure of bodies

For the moment we take the route that any body (function body or later if and loop bodies) consists of a sequence of
expressions and semicolons. We define the following rules accordingly:

 * A body may be completely empty
 * If a body is not empty, then it must start with a ';'
 * Between every two expressions must be a ';'
 * A body may be finished by an expression (without trailing ';'). In that case, this last expression defines the
   type of that body. This in turn defines the type of an 'if'-expression
 * If a body is terminated with a ';', then this body always has type 'void'
 * We treat everything as expressions, there are no statements. Even a 'let'-
   statement is treated as expression, but one that has type 'void'
