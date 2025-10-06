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
 * Improve maintainability of emitter-visitor; shouldn't be necessary
   to add every new type to the std::visit call if possible
 * implement a simple heap management concept (check the WAMR/Emscripten memory organisation)
 * Add structured types (struct, list, enum/unions)
 * Add some form of memory management
 * Add intrinsics
 * Make sure pointer types are correctly dereferenced; special checks for u8, u16, since we dereference u32 normally
 * Comparison operators and bool type
 * Check error messages when a function has it's last statement terminated with a ;
   --> this probably has to be checked with the return type and be made sure that the part after the ;
       is treated as a void return

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
