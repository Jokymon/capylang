import pytest
import subprocess
import tools


def run_compile_wat(source_path, wat_path, wasm_path) -> tuple[int, str]:
    res = subprocess.run(
        [
            "wasmtime",
            "run",
            "--dir",
            source_path.parent.as_posix(),
            "./build/capylang.wasm",
            "-i",
            source_path.as_posix(),
            "-o",
            wat_path.as_posix(),
        ],
        capture_output=True,
    )
    if res.returncode != 0:
        return res.returncode, res.stderr.decode()
    res = subprocess.run(
        ["wasm-tools", "parse", wat_path.as_posix(), "-o", wasm_path.as_posix()],
        capture_output=True,
    )
    return res.returncode, res.stderr.decode()


def run_compile_wasm(source_path, _wat_path, wasm_path) -> tuple[int, str]:
    res = subprocess.run(
        [
            "wasmtime",
            "run",
            "--dir",
            source_path.parent.as_posix(),
            "./build/capylang.wasm",
            "-i",
            source_path.as_posix(),
            "-o",
            wasm_path.as_posix(),
        ],
        capture_output=True,
    )
    if res.returncode != 0:
        return res.returncode, res.stderr.decode()
    return res.returncode, res.stderr.decode()


def pytest_generate_tests(metafunc):
    if "setup_compile_environment" not in metafunc.fixturenames:
        return
    if metafunc.definition.get_closest_marker("no_compile_matrix") is not None:
        return
    metafunc.parametrize(
        "setup_compile_environment",
        [run_compile_wat, run_compile_wasm],
        indirect=True,
        ids=["run_compile_wat", "run_compile_wasm"],
    )


@pytest.fixture(autouse=True)
def setup_compile_environment(request):
    compile_func = getattr(request, "param", None)
    if compile_func is not None:
        tools.set_compile_func(compile_func)
