import re
import subprocess

import pytest
import tools


@pytest.mark.parse_error
@pytest.mark.no_compile_matrix
@pytest.mark.parametrize("declaration", ["let value;", "let mut value;"])
@pytest.mark.parametrize("mode", ["wat", "wasm", "ast", "lir"])
def test_unresolved_variable_is_reported_before_lir(tmp_path, declaration, mode):
    source_path = tmp_path / "unresolved.capy"
    source_path.write_text(
        "@export\nfn _start() {\n    " + declaration + "\n}\n", encoding="utf-8"
    )
    output_path = tmp_path / ("output." + mode)
    options = (
        ["--dump-" + mode] if mode in ("ast", "lir")
        else ["-o", f"/tests/{output_path.name}"]
    )
    result = subprocess.run(
        [
            "wasmtime", "run", "--dir", f"{tmp_path}::/tests",
            "./build/capylang.wasm", "-i", "/tests/unresolved.capy",
        ] + options,
        capture_output=True,
        text=True,
        timeout=30,
    )

    assert result.returncode == 1, result.stderr
    assert (
        ":3:5: Cannot determine type of variable 'value'; add an explicit type annotation"
    ) in result.stderr
    assert result.stdout == ""
    assert not output_path.exists()


@pytest.mark.parse_error
def test_unresolved_variable_in_nested_scope_is_reported():
    """
@export
fn _start() {
    if (true) {
        let mut value;
    }
}
"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert "Cannot determine type of variable 'value'" in stderr
    assert "Failure in" not in stderr


@pytest.mark.good
def test_variable_type_can_be_inferred_from_later_assignment():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let mut value;
    value = 42u64;
    proc_exit((value - 40u64) as u32)
}
"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 2


@pytest.mark.good
def test_unused_typed_variable_and_replaced_type_placeholders_are_valid():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let mut unused: u64;
    let value = 42;
    let matches: bool = value == 42;
    proc_exit((matches == false) as u32)
}
"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 0


@pytest.mark.good
@pytest.mark.no_compile_matrix
def test_empty_return_has_no_untyped_placeholder_value_in_lir():
    """
@export
fn _start() {
    return;
}
"""
    lir = tools.compile_to_lir(tools.get_doc_str())

    assert re.search(
        r"_type: ReturnStatement\s+expression: null\s+is_explicit: true", lir
    ), lir


@pytest.mark.good
def test_void_return_expression_preserves_its_side_effects():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

global mut value: u32 = 0u32;

fn assign() {
    return value = 42u32;
}

@export
fn _start() {
    assign();
    proc_exit(value)
}
"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 42
