## Goals

 * general purpose programming language that only targets WASM
   --> it may use WASM (WASM component model) specific concepts
 * build a WASM compiler that runs in a WASM runtime with WASI p1
   or maybe p2

## Testing

To check the compiler source, you can run all test vectors implemented in Python using

```
uv run pytest tests/
```

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
