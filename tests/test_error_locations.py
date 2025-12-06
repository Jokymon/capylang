import tools
import pytest


@pytest.mark.parse_error
def test_location_of_incomplete_expression():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let a: u32 = 4u32+
}"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:6:23: Expected a primary (function call, number, variable)\n"
    )


@pytest.mark.parse_error
def test_undefined_variable():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    proc_exit(a_name)
}"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:6:15: Undefined variable: 'a_name'\n"
    )
