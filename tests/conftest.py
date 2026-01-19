import pytest
import subprocess
import tools


def run_compile_wat(source_path, wat_path, wasm_path) -> tuple[int, str]:
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


def run_compile_wasm(source_path, _wat_path, wasm_path) -> tuple[int, str]:
    res = subprocess.run(
        f"C:/sw/wasmtime-v33.0.0-x86_64-windows/wasmtime.exe run --dir {source_path.parent.as_posix()} ./build/capylang.wasm -i {source_path.as_posix()} -o {wasm_path.as_posix()}",
        capture_output=True,
    )
    if res.returncode != 0:
        return res.returncode, res.stderr.decode()
    return res.returncode, res.stderr.decode()


@pytest.fixture(params=[run_compile_wat, run_compile_wasm], autouse=True)
def compile_func(request):
    tools.set_compile_func(request.param)
