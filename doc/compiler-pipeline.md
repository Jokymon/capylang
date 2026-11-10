# Compiler Pipeline

The Capylang compiler currently follows a small number of explicit passes from
source text to WebAssembly output.

## Overview

```mermaid
flowchart LR
    %% Pipeline row
    parser[[Parser/Lexer]] --> typ_inf[[Type inference]]
    typ_inf --> sem_ana[[Semantic analysis]]
    sem_ana --> norm[[Normalization]]
    norm --> lir_xform[[Transformation\nto LIR]]
    lir_xform --> lir_lower[[LIR\nlowering]]
    lir_lower --> wat_gen[[WAT generation]]
    lir_lower --> wasm_gen[[WASM generation]]
    
    %% Align them horizontally
    parser ~~~ typ_inf ~~~ sem_ana ~~~ norm ~~~ lir_xform ~~~ lir_lower ~~~ wat_gen

    %% artifacts placed "below"
    AST@{ shape: document, label: "AST" }
    LIR@{ shape: document, label: "LIR" }
    WAT@{ shape: document, label: "WAT" }
    WASM@{ shape: document, label: "WASM" }

    %% Connections
    parser -.->|generate| AST
    typ_inf -.->|update| AST
    norm -.->|update| AST

    lir_xform -.->|create| LIR
    lir_xform <-.-|read| AST
    lir_lower -.->|update| LIR

    wat_gen -.->|create| WAT
    wat_gen <-.-|read| LIR
    wasm_gen -.->|create| WASM
    wasm_gen <-.-|read| LIR
```

This is the current logical pipeline. Individual implementation details may
still move while the compiler grows, but these stages describe the existing
compiler structure.

## Parser

The parser consumes lexer tokens and constructs the AST used by later stages.
At this stage the compiler preserves source-level structure, including syntactic
sugar that is still convenient for later semantic work.

## Type Inference

Type inference resolves types that are introduced implicitly by variables,
expressions, and blocks. The pass is responsible for turning partially typed
program fragments into a form where later checks can rely on explicit type
information.

## Semantic Analysis

Semantic analysis verifies that the program is internally consistent after type
inference. In particular, it checks that:

1. No unknown types remain unresolved.
2. `if` branch result types agree with each other and with the target type.
3. Assignment source and destination types match.
4. Returned values match the enclosing function signature.

## Normalization

Normalization removes source-level special cases that would otherwise leak into
lowering and code generation. The current normalizations include:

 * Converting a final expression without a trailing semicolon into an explicit
  `return` expression.
 * Add explicit pointer dereferencing nodes for record field accesses where they
  are missing and where the field access is on a pointer to a record

This stage is where additional syntactic sugar should be lowered into more
uniform internal forms.

## Transformation to LIR

The normalized and type resolved AST is converted into a language-specific
intermediate representation, short LIR. This representation is chosen to
simplify the generation for some of the WASM language constructs and mainly
turns let-statements and assignment expressions into corresponding load 
expressions and store statements.

## LIR Lowering

Similar to the "Normalization" step for the AST, "LIR Lowering" transforms some
of the more higher level LIR node types into lower level constructs. The current
lowerings include:

 * Convert store_record_statements into multiple store_statement which set the
   individual fields of the record according to the initialisation list

## WASM IR Emission

After semantic work is complete, the compiler emits an internal representation
that is close enough to WebAssembly to support backend code generation.

Today this stage is described at a high level only. As the internal IR becomes
more stable, this section should be expanded with details about the emitted
structures, ownership of type information, and lowering conventions.

## WAT and WASM Generation

The final backend stages turn the lowered representation into textual WAT and
binary WASM output. Both backends are part of the supported compiler surface and
need to remain aligned when new language features are added.
