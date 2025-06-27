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

 * operator precedence
 * add other operator types
 * Improve maintainability of emitter-visitor; shouldn't be necessary
   to add every new type to the std::visit call if possible
 * Add variable parameter count for function calls