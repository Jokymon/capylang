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
def test_let_doesnt_need_initialiser_expression():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn _start() {
    let mut a: u32;
    a = 17u32;
    proc_exit(a)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 17


@pytest.mark.good
def test_pointer_dereferencing():
    # For this test, the data in the linear memory is hardcoded in the
    # compiled WAT file. That data starts at address 100
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn _start() {
    let a: *u32 = 104u32 as *u32;
    proc_exit(*a)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 0x10


@pytest.mark.good
def test_pointer_deref_on_lhs():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn _start() {
    let a: *u32 = 100u32 as *u32;
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


@pytest.mark.parse_error
def test_mismatching_types_in_let():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn _start() {
    let a: u32 = 100s32;
    proc_exit(a)
}"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:5:5: Type mismatch in let statement. Variable is of type 'u32' but expression has type 's32'\n"
    )


@pytest.mark.good
def test_let_without_any_assignment_before_usage_is_error():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn _start() {
    let mut a: u32;
    proc_exit(a)
}"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:6:15: Variable 'a' used without assigning a value before\n"
    )
