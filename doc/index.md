# Capylang Compiler Internals

This section documents how the Capylang compiler is organized internally and
which design choices currently shape the implementation.

The published documentation is intentionally small for now and focuses on the
core compiler pipeline:

- [Charta for the compiler](charta.md) gives the guiding principles and core ideas for this project
- [Compiler pipeline](compiler-pipeline.md) describes the major compilation
  stages and the responsibilities of each pass.
- [Places and storage locations](places-and-locations.md) describes the concept of a place and of storage
  locations as used by LIR and the LIR-emitter in more details.
- [Design decisions](design-decisions.md) records architectural choices that
  affect the language and compiler implementation.
- [Roadmap](roadmap.md) collects open compiler work that is relevant for future
  implementation and documentation.
- [Journey](journey.md) describes some of the bigger milestones and roadblocks encountered along the way
