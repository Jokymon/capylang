# Capylang Compiler and Language

Welcome to the Capylang (so far toy) programming language. This goal of this
language and ecosystem is to provide a programming experience that buys
completely into the WebAssembly environment. The compiler and eventually all the
surrounding tooling should be solidly based on WebAssembly and possibly even the
component model with Wasi Preview 2 or newer.

It is explicitly not a goal of Capylang to provide backends for any other
target, including any native targets like X86, ARM, RISC etc. or another VM.
This may proof to be more challenging due to at places still limited support for
certain system functions in WASI but will hopefully also make tasks like backend
implementation easier, because we can fully rely on the functionality provided
by the WASM instruction set and environment.

## Goals

 * general purpose programming language that only targets WASM
   --> it may use WASM (WASM component model) specific concepts
 * build a WASM compiler that itself runs in a WASM runtime with WASI p1
   or maybe p2
 * provide a WASI-based development environment similar to `cargo`

## Getting started

## Building

To compile any capylang source files, you first need to compile the C++-based
Capylang compiler. Currently this relies on Clang-C++ and CMake. The compiler
itself is run using the wasmtime runtime. So make sure, you have the following
tools installed and available in the `PATH`:

 * CMake, >= 4.2.0
 * Wasmtime, >= 41.0.0
 * Wasm-tools, >= 1.244.0
 * Ninja, >= 1.13.0     (or alternatively Make or any other CMake backend)

Additionally CMake will use `cmake/wasm-wasip-clang.cmake` as configuration for
the toolchain. Install the following additional tools and adjust the paths in
`cmake/wasm-wasip-clang.cmake` according to your installation paths:

 * Wasi-sysroot, >= 29.0
 * Wasi-SDK, >= 29.0

By default the toolchain file looks for these locations:

 * Windows: `C:/sw/wasi-sysroot-29.0` and `C:/sw/wasi-sdk-29.0-x86_64-windows/bin`
 * Linux: `/opt/wasi-sysroot-29.0` and `/opt/wasi-sdk-29.0/bin`

You can override both explicitly with environment variables:

 * `CAPYLANG_WASI_SYSROOT`
 * `CAPYLANG_WASI_SDK_BIN`

With all the tools installed run the following commands once to initialize the
build directory:

```bash
mkdir build
cd build
cmake -G"Ninja" --toolchain ../cmake/wasm-wasip-clang.cmake ..
```

And finally build the CMake project from inside the `build/` directory:

```bash
cmake --build .
```

Now the compiler is ready for use in `build/capylang.wasm` and can be run with
any WASM runtime. Remember to give the Capylang compiler some access rights to
the local filesystem to read source files and to generate WAT and WASM files.
For example using wasmtime you would run the following command to compile the
example source file:

```bash
wasmtime run --dir . build/capylang.wasm -i example.capy -o example.wasm
```

The generated `example.wasm` is again a runnable WASI module that can be run
with a WASI-capable WebAssembly runtime:

```bash
wasmtime run example.wasm
```

### Building the documentation

The compiler internals documentation is written in Markdown under `doc/` and is
built as a small MkDocs site. To generate the HTML version locally, run:

```bash
uv sync --group docs
uv run mkdocs build --strict
```

This writes the generated site into `site/`. To preview the documentation
locally with a development server, run:

```bash
uv run mkdocs serve
```

## Testing

To check the compiler source, you can run all test vectors implemented in Python using

```bash
uv run pytest tests/
```

Additionally a set of C++-based unittests are found in the `src/tests` folder.
These tests are based on the Catch-2 framework and are also built with CMake.
They can also be run with a WebAssembly runtime, for example by running

```bash
wasmtime run build/capylang-unit-tests.wasm
```

## Working with the ANTLR grammar

Make sure, you have installed all the dependencies according to `pyproject.toml`
and run the Python-based `antlr4` tool one time a version number to install the
necessary Java files:

```bash
uv run antlr4 -v 4.13.2
```

Newer ANTLR4 versions might work, but 4.13.2 was successfully used and has
proven to work.

Now to build the Python-based parser code, run

```bash
uv run antlr4 -Dlanguage=Python3 ./grammar/CapylangParserGrammar.g4 ./grammar/CapylangLexerGrammar.g4
```

## Capylang VSCode extension

The folder `./capylang` consists of a sub project which represents the code for a Capylang language extension to be
used in VSCode.

To build the package, make sure, you have the NPM package `vsce` installed through `npm install -g @vscode/vsce`, then
run

```bash
cd capylang
vsce package
```

Once a package is built and the VSIX file is present in the `./capylang` folder, you can install it in your VSCode from
the command line using

```bash
code --install-extension ./capylang-......vsix
```

Make sure, to replace the dots `.....` with the correct version number. After that, VSCode should be restartet for the
any changes in the extension to take effect.

## Playground

The directory `playground` contains a simple web application and a Python-based
script for a simple webserver to run the generated example in the browser. 
Currently the web application only runs the web assembly module called
`example.wasm` in the root directory. To start the server just run

```bash
uv run playground/playground.py
```

Then open the web page with the given URL. The use should be self explanatory ;-)

### Drawing API

The playground provides a drawing API towards the WebAssembly module. All the
functions are provided through the `canvas` module. The following functions are
implemented so far:

 * `width() -> u32`; for getting the width of the canvas.
 * `height() -> u32`; for getting the height of the canvas.

The playground itself also expects exported functions in the loaded WASM module.
So far these are:

 * `init()`; This function is called by the playground once at the start of the
   application and can be used to initialise global variables, allocate required
   memory and set any other initial values as needed.
 * `create_frame() -> u32`; This function is called by the playground to request
   a new "frame". The WASM module should reserve an area in an exported linear
   memory which it can use to draw on. Each pixel takes 4 bytes to represent.
   The 4 bytes represent the colors R, G, B, followed by the alpha channel in
   the last byte. The R-value is stored in the lowest address, followed by the
   next byte representing the G-value etc. etc.
   The returned value shall be the pointer to the start address of this area.
 * `on_event(eventid: u32, data1: u32, data2: u32, data3: u32, data4: u32)`;
   This function is called by the playground to forward UI events to the running
   WASM instance. So far the following events with respective event ids are
   available:

    * *0001*: Mouse click where x-position is `data1`, y-position is `data2`.

## Contributing

Contributions are welcome and will be considered. However this is a carefully
curated, handcrafted compiler project grown out of passion for the ideas and
concepts. Contributions in the form of Pull-Requests will be reviewed with
similar attention to detail and the author might ask you for some adjustements
in your PR before accepting it.

It is therefore also recommended to consider smaller modifications, extensions
and additions rather than trying to push one large
"change-and-improve-everything" PR which is hard to review.

The following guiding principles will more likely get your PR accepted:

 * The project still mostly uses C++ for the implementation of the compiler.
   This is intended to change once the compiler is capable enough to host itself
   but until then, that's how it is. For C++ stick to the following rules:
   * Do not use/rely on exceptions. The WASM-backend Clang doesn't (yet) support
     exceptions
   * Use templates more as type-generics and restrain from extensive template
     meta programming. This should keep the source more approachable for new
     contributers.
   * All commonly available features of C++26 language and STL may be used
     except where already explicitly excluded in the points above.
 * For every new language feature make sure to also add at least one or two
   good case tests and if appropriate at least one parser/semantic check/...
   failure case.
 * For new infrastructure code check if it makes sense to also add unittests and
   if yes, make sure to add them.
 * Make sure to consider the WAT _and_ the WASM backends when adding a feature.
   The Python-based parser tests already run with both backends, but if you add
   functionality not covered by these tests, still make sure to also consider
   both backends.
 * Use clang-format to get consistent formatting of your source code. The
   required clang-format version is given at the top of the .clang-format file.
 * Use the CAPY_FAIL and CAPY_ASSERT macros for any situation that should really
   not happen. In a lot of cases "should not happen" means, that a previous
   compiler pass should have taken care that this situation doesn't occur.
