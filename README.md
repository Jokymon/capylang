## Goals

 * general purpose programming language that only targets WASM
   --> it may use WASM (WASM component model) specific concepts
 * build a WASM compiler that runs in a WASM runtime with WASI p1
   or maybe p2

## TODO

 * testing infrastructure
 * formalizing the grammar
   --> derive syntax highlighters and a language server
 * document or even automate the setup of the development tools
   --> WASI SDK + Sysroot
   --> Wasm tools
   --> Wasmtime or other WASM runtimes

 * Parser error locations in output
 * Support for multiple operators
 * ignoring whitespaces around operators
 * operator precedence