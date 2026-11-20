# Roadmap selfhosting

One of the bigger goals of this project is to be completely self host the entire compiler. This means that we 
will eventually replace all current C++ code with Capylang code, build the compiler using a pre-compiled version 
and run all of this completely in a WebAssembly environment.

To get to this point, we will first have to clarify a few important questions and implement some basic 
functionality. The necessary steps are outlined in this document.

In short selfhosting depends on

 * a concept for modularizing a larger software project, potentially involving at least some basic tooling
 * a string library with support tokenizing large text
 * support for basic file-based IO to read source files and write WAT and binary files
 * structured types like records, arrays, variants
 * powerful control structures for looping over elements of a container or deciding over choices of a variant