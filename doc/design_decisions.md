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
