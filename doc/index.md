# Capylang Compiler

The Capylang compiler is an experiment at implementing a new compiler tightly integrated with the WebAssembly ecosystem.
It fully relies on WebAssembly runtimes to run and makes targetting other platforms like X86, ARM, RISC or other VMs an
explicit no-goal.

The compiler and its ecosystem are equally intended as a playground for students to learn and explore modern compiler
design as well as for a small research team to explore the options in this focused design space. More details about the
goals of this projects can be found in the [Charta for the compiler](charta.md).

If you are interested in how far the compiler has already come and what decisions were made, you can check

- [Journey](journey.md) describes some of the bigger milestones and roadblocks encountered along the way
- [Design decisions](design-decisions.md) records architectural choices that
  affect the language and compiler implementation.

If you are interested in the plans for the future of the compiler, check the roadmap documents

- [Roadmap](roadmap.md) collects open compiler work that is relevant for future
  implementation and documentation.
- [Roadmap selfhosting](roadmap-selfhosting.md) outlines the mid to longer-term roadmap to
  get to a selfhosting compiler.

## Compiler details

- [Compiler pipeline](compiler-pipeline.md) describes the major compilation
  stages and the responsibilities of each pass.
- [Places and storage locations](places-and-locations.md) describes the concept of a place and of storage
  locations as used by LIR and the LIR-emitter in more details.
- [Checklists for adding new features](checklists.md) gives comprehensive lists of items to
  check before declaring a new Capylang language feature as "completed".
