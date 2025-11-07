import tools
import pytest


@pytest.mark.good
def test_let_statement_with_simple_type():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn _start() {
    let a: u32 = 2u32;
    proc_exit(a)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 2


@pytest.mark.good
def test_pointer_dereferencing():
    # For this test, the data in the linear memory is hardcoded in the
    # compiled WAT file. That data starts at address 100
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn _start() {
    let a: *u32 = 104u32;
    proc_exit(*a)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 0x10


@pytest.mark.good
def test_pointer_deref_on_lhs():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn _start() {
    let a: *u32 = 100u32;
    *a = 45u32;
    proc_exit(*a)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 45


@pytest.mark.good
def test_non_pointer_deref_on_lhs():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn _start() {
    let mut a: u32 = 100u32;
    a = 45u32;
    proc_exit(a)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 45


# ######## FAILURE HANDLING ###########
@pytest.mark.parse_error
def test_missing_type_spec_for_let():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn _start() {
    let a: = 100u32;
    a = 45u32;
    proc_exit(a)
}"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:5:11: Expecting an identifier for the type specification\n"
    )
