# Design Decisions

This page collects design choices that influence the compiler internals and the
language model. The goal is to capture the reasoning behind decisions that are
easy to lose once the implementation evolves.

## Testing Strategy

Compiler behavior is primarily tested with Python and `pytest`. This gives the
 project a flexible way to define source-level scenarios, compiler invocations,
 and output checks.

The repository uses `uv` to simplify Python environment and test execution.

## Code Generation and Binaryen

Binaryen would be an attractive code generation backend because it can emit both
textual WAT and binary WASM and also provides optimization passes. The project
experimented with that route but postponed adoption for now.

The current blockers are practical integration issues with the existing
toolchain:

- Binaryen expects exception support, while the Clang-based WASI SDK setup used
  by the project still does not provide a clear path for enabling exceptions.
- Binaryen expects thread support, which likely requires switching the target to
  `wasm32-wasip1-threads` and then validating the rest of the toolchain.

The result is a deliberate decision to keep code generation in-project until the
toolchain constraints are better understood.

## Strings

The `string` type represents UTF-8 encoded Unicode text.

For explicit code point escapes, the language uses the flexible `\u{...}`
notation known from languages such as Rust, Swift, and PHP instead of fixed
width `\u` or `\U` escape formats.

### String Literal Storage

String literals are stored in the parse context structure together with symbol
and type information.

This keeps literals available to later compiler stages without tying them to the
AST alone. If later lowering stages operate on an IR instead of the AST, string
literal access remains straightforward and does not require duplicating that
data into multiple representations.

## Body Structure

Function bodies, and later also `if` and loop bodies, are modeled as sequences
of expressions separated by semicolons. Capylang currently treats everything as
expressions rather than mixing statements and expressions.

The current rules are:

- A body may be empty.
- A non-empty body starts with a semicolon.
- Every pair of adjacent expressions is separated by a semicolon.
- A body may end with an expression that has no trailing semicolon. In that
  case, that expression defines the body type.
- A body ending with a semicolon always has type `void`.
- `let` is modeled as an expression with type `void`.

This keeps the type model uniform and directly supports expression-oriented
constructs such as typed `if` expressions.

## Symbol Handling

The compiler uses one shared symbol concept rather than separate lookup tables
for variables, functions, and types. The concrete symbol kind is represented by
a symbol tag.

Expected advantages:

- Better diagnostics when a lookup finds a symbol of the wrong kind, such as
  calling a variable as though it were a function.
- Easier extension when new symbol kinds are introduced.

Tradeoff:

- Users of the symbol table need an additional check after lookup to confirm the
  expected symbol kind.

## Attributes

Attributes mark functions and, later, potentially other language elements. The
syntax follows a Python-like style with identifiers that start with `@`, one
attribute per line.

Simple attributes are single identifiers. More complex attributes may use a
function-like syntax with named parameters, but parameter values are restricted
to simple literals such as booleans, numbers, characters, and strings. Arbitrary
expressions are intentionally excluded to keep attribute handling simple.

Currently recognized builtin attributes:

- `@export` marks a function for export from the generated WASM module.

## Language IR (LIR)

The language intermediate representation, or LIR, is loosely inspired by Rust
MIR. Roughly half of the AST node shapes can be represented directly, but some
areas intentionally change in LIR.

### Explicit Load and Store Operations

LIR introduces explicit `load` expressions and `store` statements. These can be
understood as IR-only operations that work on a `place` abstraction rather than
on source-level expressions alone.

A `place` can represent:

- A variable
- Field access
- Pointer dereference

This is meant to simplify backend lowering because the emitter can distinguish
read and write contexts directly instead of reconstructing them from general AST
expressions and LHS/RHS context in assignments.

### Record Initialization Lowering

Record initializers are simplified into a structure that contains:

- The record type
- The initialization expressions in field-definition order

### String Lowering

String lowering is currently still a bit sketchy. When a string literal is used
in a let-statement, then a special `store_string` LIR node is generated. We
could have mapped this to a `store_record_statement`, but there is a problem
with the field `ptr` of the string. This field only gets the correct/actual
value in the final emitter-phase when we know the addresses of the string
literals. So we would have had to treat any `store_record_statement` special,
but only if it's type is a builtin string type.
Currently this `store_string` node is only generated when we are in a `let`-
statement, but not in an assignment expression. Fixing the assignment expression
would be easy, but maybe we should start refactoring this part first into a
common function block for assignments.
Additionally, `store_string` is only generated, when we have a
`node_string_literal` as RHS. Currently this is no problem as we do not provide
any string-based operators or even string returning functions. But once we get
to that point, we might have to make this check more intricate.

### Type Definitions Outside LIR

Type definitions are not carried directly in LIR. Instead, LIR relies on the
symbol table as the source of truth for type information.

### Possible Future Changes

- After type inference, unresolved type variables could be removed so that LIR
  only carries fully reified types.
- Record field initializers may eventually need temporary values to preserve
  source evaluation order when field definition order differs from code order.

## Record lowering

Records are representable in two forms, (1) the "value-based" representation and
(2) the purely pointer-based represenation. When specifying a variable to be
of a plain record type, then the first form is used. In this form, lowering
happens according to the rules of `flatten_functype()` described in the
component model "CanonicalABI". This means that some records might be lowered
into a list of local variables while bigger records will be lowered into linear
memory based storage.
When a variable is specified as pointer to a record, then the entire record will
always be represented in linear memory and the corresponding memory will be
automatically allocated at the time of record initialisation.

TODO: When exactly will this memory be freed again? Should we just treat it all
as "value-semantics", would that help in deciding when to throw away?

The first form is useful when adhering to the component level specification and
can optimize record field access when we don't need to dereference record
fields. The second form on the other hand might be needed when adopting WASI
APIs which use pointers even to smaller records.

How should the record initialisation for pointer-based records look like?
Options:
 * **(1)** Just make it look like "normal" record initialisations and let the
compiler introduce any necessary helper-nodes which tell the emitter/backend
that a memory allocation is needed.
 * **(2)** Introduce some special syntax marking the `rectype{}` "constructor" as
something that should return a pointer.
   * **(a)** Use a symbol to mark a pointer based creation, such as `^rectype{}`
     `*rectype{}` or even as postfix symbol to the type.
   * **(b)** Use a keyword to indicate the process of allocation, like `new`,
     `alloc` or similar.
   * **(c)** Use an explicit function call with some form of generic type as in
     `create<rectype>(...)` or `allocate<rectype>(...)`.

### Decision

For the moment we start with a simple, straight-forward and established concept.
We use option **2b** which makes the initialisation as pointer explicit. This
means that we don't have to introduce some weird implicit structure generation
in the background and make the whole process clearly visible to the programmer.

On the other hand, though it is a simple syntactic model, it introduces a bunch
of AST, and other constructs in the whole compiler which might have to be
removed again in the future. Also, this syntax with a prepended keyword breaks
out of the current concept of functions with parameters and thus introduces a
new precedent for syntax constructs, that we might not want to keep longterm.

==> The big challenge is, that we now have a mix of pointers and records when
    looking at field dereferences. This is currently addressed through the
    helper function `record_behind()` in the `context` class.

### Implementation steps

1. Migrate the current code base to this new concept such that all existing
   record uses are actually of form 2, the pointer based approach. That is what
   is currently assumed anyways. **DONE**
2. Add support for value-based record representation. This is not yet supported
   and would need to be implemented. **to be tested**
3. Migrate the currently one-of "scalar"-type string to this new value-based
   record type. **DONE**

## Data type definition

For the definitions of data types, used in tree representations, we want to move
to project [silwright](https://pypi.org/project/silwright/). Using this project
we can describe the used node types in a programming language independent form
and generate all necessary code automatically. The first step is done for the
introduction of the LIR.

The existing usage of the X-Macro based definition of the AST-nodes is planned
to be replaced with an ndef definition in the midterm as well to create a
consistent experience in the project code.

The main motivation of this change was the growing complexity of maintaining and
extending the X-Macros. The bigger and bigger snippets of code that somehow got
stitched together with multiple layers of macro code didn't help readability and
a new approach was in need.