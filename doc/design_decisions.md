# Language and compiler design decisions

## Testing

To test the correct behavior of the compiler infrastructure and the implemented grammar/semantics, we use a
Python/pytest-based approach. Writing tests in Python provides a lot of possibilites and flexibility to not only run
various test scenarios but also provide other means to work with the test sources used in the tests.

To simplify working with Python and pytest even further, the command line tool `uv` is used.

## Using Binaryen as code generator

Binaryen would be a good generator to easily produce WASM WAT format but also binary WASM modules. Additionally it would
even include some optimisation steps out of the box. However during first experiments of adding the project as CPM
packet, several problems were encountered and the idea was ultimately abandoned for the moment.

 * Binaryen needs support for exceptions; We currently use the Clang-based WASI SDK which as of 23.10.2025 still claims
   to not support exceptions. The option `-fno-exception` could be removed from `CMakeLists.txt` but it is unclear
   whether this really works. Some compiler errors still showed up.
 * Binaryen needs threads support; With a simple switch to target `wasm32-wasip1-threads` this should be fixable but
   also here might be more problems than initially seen.

## String implementation

The datatype `string` contains UTF-8 encoded Unicode text.

To insert specific unicode code points in strings, we use the more flexible notation `\u{xxx}` as used in Rust, Swift or
PHP rather than the stricter, fixed length variants with `\u` and `\U` as in Java, C, C++ or C#.

### String literal handling

String literals are stored in the parse context structure which already contains all symbols and types.

The advantage of this approach is that it's easier for other compiler stages to access the string literal list. We could
store the string literals in the `node_module` AST node as initially implemented, but as soon as we want to use these
literals in other compiler stages which may no longer rely on the AST but maybe on some form of IR, we will have to
duplicate this information somehow.

## Encoding Unicode code points

To insert specific unicode code points in strings, we use the more flexible notation `\u{xxx}` as used in Rust, Swift or
PHP rather than the stricter, fixed length variants with `\u` and `\U` in Java, C, C++ or C#.

## Structure of bodies

For the moment we take the route that any body (function body or later if and loop bodies) consists of a sequence of
expressions and semicolons. We define the following rules accordingly:

 * A body may be completely empty
 * If a body is not empty, then it must start with a ';'
 * Between every two expressions must be a ';'
 * A body may be finished by an expression (without trailing ';'). In that case, this last expression defines the
   type of that body. This in turn defines the type of an 'if'-expression
 * If a body is terminated with a ';', then this body always has type 'void'
 * We treat everything as expressions, there are no statements. Even a 'let'-
   statement is treated as expression, but one that has type 'void'

## Symbol handling

Rather than having separate lookup tables for different symbol types (variables, functions, types, ...) we keep one
common concept of symbols that we manage in a table. Then we distinguish different types of these common symbols using
a symbol tag.

We expect the following advantages:

 * We might be able to give better error messages when trying to call a variable or a type, because we could first try
   to lookup a symbol and then check if it even is a function symbol. If it is not, we could even inform the programmer
   about what type of symbol we found instead.
 * We can more easily introduce new kinds of symbols by just extending the symbol tag.

The disadvantage is, that we might have to take some additional steps, to check, that a found symbol actual has the
kind we need. But we currently expect this to cause minor issues when compared to the advantages.

## Attributes

We use the concept of attributes to mark functions and later potentially also other elements. We use the Python-like
notation with identifiers starting with `@` and keeping every attribute on its own line. Simple attributes can just
be one identifier, complex attributes could look like function calls with named parameters, but the parameter values
can only be simple values like bools, numbers, characters or strings. Other, more general expressions shall not be
supported to keep this part of the compiler simple.
Compiler-recognized/builtin attributes are:

 * `export` to mark a function as exported from a WASM module

## First attempt at an intermediate representation

The language intermediate representation (LIR) is roughly based on the MIR used in Rust. About half of the nodes
defined in the AST have identical representations in LIR. However the following aspects are explicitly handled
differently in LIR:

 1. The LIR introduces the concept of *load* and *store* expressions which can be seen as special functions only known
    by the LIR. These functions should reduce the challenges from having to track LHS and RHS contexts when dealing with
    variables, field access and pointer dereferencing. Instead of simply dealing with a variable, these functions
    refer to something called a `place`, which not only includes single variables, but can also contain field access
    and pointer dereferencings. By knowing that something is a load or a store and by having the full context of the
    referenced `place`, the emitter should have it easier to generate the corresponding webassembly code.
 2. Record initialisations are simplified into a struct which only contains the records type and initialisation
    expressions in field definition order.
 3. Type definitions are no longer visible in the LIR, they are instead taken from the symbol table.

Possible future modifications:

 * The transformation to LIR happens after type inference, so all the necessary type information should already be
   established. One future modification could be, the remove the version of an unresolved type variable and to make
   sure that everywhere only the reified types are available.
 * Currently the order for the evaluation of record field initialisation expressions is modified by LIR to match the
   definition order. This could be unexpected when a programmer relies on side effects and the correction execution
   order. We might have to introduce some form of LIR-level temporaries to first evaluate field initialisers in code
   order, assign them to these temporaries and then actually initialise the record itself using the temporaries.