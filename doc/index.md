# Capylang Compiler Internals

This section documents how the Capylang compiler is organized internally and
which design choices currently shape the implementation.

The published documentation is intentionally small for now and focuses on the
core compiler pipeline:

- [Charta for the compiler](charta.md) gives the guiding principles and core ideas for this project
- [Compiler pipeline](compiler-pipeline.md) describes the major compilation
  stages and the responsibilities of each pass.
- [Design decisions](design-decisions.md) records architectural choices that
  affect the language and compiler implementation.
- [Roadmap](roadmap.md) collects open compiler work that is relevant for future
  implementation and documentation.

The repository also contains additional draft material under `doc/`, such as
`lir_examples.md`, imported component model references, and idea sketches. Those
are currently treated as working notes and are not part of the published site.
