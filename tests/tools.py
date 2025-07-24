import pathlib
import os
import subprocess
import tempfile


def run_compile(source_path, wat_path, wasm_path) -> tuple[int, str]:
    res = subprocess.run(
        f"C:/sw/wasmtime-v33.0.0-x86_64-windows/wasmtime.exe run --dir {source_path.parent.as_posix()} ./build/capylang.wasm -i {source_path.as_posix()} -o {wat_path.as_posix()}",
        capture_output=True,
    )
    if res.returncode != 0:
        return res.returncode, res.stderr.decode()
    res = subprocess.run(
        f"C:/sw/wasm-tools-1.230.0-x86_64-windows/wasm-tools.exe parse {wat_path.as_posix()} -o {wasm_path.as_posix()}",
        capture_output=True,
    )
    return res.returncode, res.stderr.decode()


def run_compile_error(source_path, wat_path) -> tuple[int, str]:
    res = subprocess.run(
        f"C:/sw/wasmtime-v33.0.0-x86_64-windows/wasmtime.exe run --dir {source_path.parent.as_posix()} ./build/capylang.wasm -i {source_path.as_posix()} -o {wat_path.as_posix()}",
        capture_output=True,
    )
    return res.returncode, res.stderr.decode()


def run_wasmfile(wasm_path) -> tuple[int, str]:
    res = subprocess.run(
        f"C:/sw/wasmtime-v33.0.0-x86_64-windows/wasmtime.exe run {wasm_path}",
        capture_output=True,
    )
    return res.returncode, res.stdout.decode()


def run_test_code(source_code) -> tuple[int, str]:
    """Compiles the given source_code into a WASM module
    and run it in a WASM runtime. The returned values
    represent the exit code of the run code and the
    stdout for running it."""
    source_file = tempfile.NamedTemporaryFile(mode="w", encoding="utf-8",
                                              delete=False)
    source_file.write(source_code)
    source_file.close()

    source_file_path = pathlib.Path(source_file.name)
    wat_file_path = source_file_path.with_suffix(".wat")
    wasm_file_path = source_file_path.with_suffix(".wasm")

    exit_code, stderr = run_compile(source_file_path, wat_file_path,
                                    wasm_file_path)
    if exit_code != 0:
        raise ValueError("Compilation unexpectedly failed:\n" + stderr)
    exit_code, stdout = run_wasmfile(wasm_file_path)

    os.remove(source_file.name)
    if os.path.exists(wat_file_path):
        os.remove(wat_file_path)
    if os.path.exists(wasm_file_path):
        os.remove(wasm_file_path)

    return exit_code, stdout


def compile_test_code(source_code) -> tuple[int, str]:
    """Compiles the given source_code into a WASM module
    The returned values represent the exit code of the
    compile execution including the run code and the
    stderr."""
    source_file = tempfile.NamedTemporaryFile(mode="w", encoding="utf-8",
                                              delete=False)
    source_file.write(source_code)
    source_file.close()

    source_file_path = pathlib.Path(source_file.name)
    wat_file_path = source_file_path.with_suffix(".wat")

    exit_code, stderr = run_compile_error(source_file_path, wat_file_path)

    os.remove(source_file.name)
    if os.path.exists(wat_file_path):
        os.remove(wat_file_path)

    return exit_code, stderr


def expression_to_full_program(code):
    procedure_wrapper = """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn _start() {{
    {code}
}}"""
    full_code = procedure_wrapper.format(code=code)
    return full_code


def normalize_filename_from_output(stdxxx):
    print(stdxxx)
    if stdxxx.lower().startswith("c:/"):
        stdxxx = stdxxx[3:]

    idx = stdxxx.find(":")
    if idx == -1:
        return stdxxx
    else:
        return "filename" + stdxxx[idx:]
