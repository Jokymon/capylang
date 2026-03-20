## TODO
 * formalizing the grammar
   --> derive syntax highlighters and a language server
 * document or even automate the setup of the development tools
   --> WASI SDK + Sysroot
   --> Wasm tools
   --> Wasmtime or other WASM runtimes

 * add 64-bit version of numbers
 * add floating point number types
 * add support for hexadecimal prefixes
 * Add unicode code point support for characters (so far we only have it in strings)
 * design and add support for vector numeric types
 * add structured component model types (variant/enum, list, result, ...)
 * Extend the basic memory management
 * Make sure pointer types are correctly dereferenced; special checks for u8, u16, since we dereference u32 normally
 * design and implement a concept of symbol visibility
 * 'address of'-operator and where it should be supported; do we really want to go the path of C, where a local
   variable always needs to be addressable? Maybe we can just not take an address of every variable, maybe that is
   just a property of some data types
 * In some cases the parser seems to get stuck in an endless loop trying to look for some token that never appears
 * Add an option to the compiler to just dump the sequence of tokens
 * Add --help option to the compiler
 * Add intrinsics
 * Error messages from diagnostics bag don't seem to contain the file name; Also the "Returned value of type ... doesn't
   match the declared return type ..." message seems to always be reported on line 1 column 1. 

### Implement an (H)IR

Currently the code contains some experiments with an ANF-based intermediate representation that was created with the
help of Codex. However it turned out that this was created without a clear understanding of the needs from the backend
side and either needs a lot of additions and changes or we should anyways completely rethink the concept for an IR that
actually fits our needs for efficient WASM-IR generation.

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
    --> COMPLETED; all non-expression nodes are now handled outside of `ast_node`
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
