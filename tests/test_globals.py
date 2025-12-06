import tools
import pytest


@pytest.mark.good
def test_support_for_globals():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

global value: u32 = 10u32;

@export
fn _start() {
    proc_exit(value)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 10


@pytest.mark.good
def test_mutable_globals_can_be_modified():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

global mut value: u32 = 10u32;

@export
fn _start() {
    value = 11u32;
    proc_exit(value)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 11


# ----------------------------------------
# syntactic errors
@pytest.mark.parse_error
def test_immutable_globals_cant_be_modified():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

global value: u32 = 10u32;

@export
fn _start() {
    value = 11u32;
    proc_exit(value)
}"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:8:5: Can't assign to immutable variable 'value'\n"
    )


@pytest.mark.parse_error
def test_globals_must_be_initialised():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

global value: u32;

@export
fn _start() {
    proc_exit(value)
}"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:4:18: Globals need an initialisation value\n"
    )
