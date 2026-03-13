## Compiler passes

1. Parser (help by lexer)
2. Type inference
3. Semantic analyzer
4. Normalization
5. WASM-IR emitter
6. WAT/WASM generation

### Type inference

This pass trys to resolve any types that are only implicitly assigned to variables and blocks.

### Semantic analyzer

This pass checks the consistency of the types assigned during the previous steps. In detail it checks

 1. All unknown types are resolved
 2. All return values of `if`-branches are equal and
    match the target type
 3. All types used in assignments match
 4. Types of values returned from functions match the
    function signatures return types

### Normalization

This step handles omissions and special cases coming from syntactic sugar that is still in the AST at this point. Normalizations are

 * The final expression without a terminating semicolon in a function is turned into a return expression