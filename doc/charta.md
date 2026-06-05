# Capylang guiding principles

Capylang is a research language targeting the WebAssembly ecosystem. 

## Technical goals

 * Capylang is rooted in the WebAssembly world. It should make deliberate use of WebAssembly’s capabilities and address its limitations through language and tooling design.
 * WebAssembly is not just the output format, but part of the development model:
   * The primary, and currently only, compilation target of Capylang is WebAssembly, both in binary form and in text form (WAT).
   * The compiler itself should run in WASI-compatible WebAssembly runtimes.
   * The goal is to have first-class support for diagnostics, testing, and a language server.
   * WebAssembly depends on a runtime environment with a very specific interface through exported functions. The Capylang concept can also address aspects beyond the pure language including integration in host environments.
 * Language design decisions should be informed by modern programming language concepts, while remaining practical to implement and reason about.
 * The primary language paradignm is imperative with selective features of OO and FP added where useful.

## Research goals

 * Explore the challenges of using WebAssembly across frontend and backend environments, and, with lower priority, in more constrained or experimental environments such as embedded runtimes.
 * Investigate how a programming language can be designed to collaborate well with LLMs, for example by reducing token usage, reducing ambiguity, and making generated code and reviews easier for both humans and machines.
 * Use Capylang as a vehicle for studying the interaction between language design, intermediate representations, tooling, and WebAssembly execution models.
 * Explore the potential of an ecosystem that uses WebAssembly specifics to implement security features such as fine grained capability permissions.

## Educational goals

 * Enable students to work on a non-trivial compiler project and encounter the practical challenges of modern compiler construction.
 * Make design decisions explicit and discussable, so the project can serve as a teaching tool for language design, compilation, and runtime systems.
 * Prioritise understandable internals over premature complexity.
