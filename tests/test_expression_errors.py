import tools
import pytest


@pytest.mark.parse_error
def test_number_with_illegal_suffix():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn _start() {
    proc_exit(4xy)
}"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:5:18: Number has an illegal suffix\n"
    )


@pytest.mark.parse_error
def test_multiple_numbers_with_illegal_suffix():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn _start() {
    let a: u32 = 5ab;
    proc_exit(4xy+a)
}"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:5:21: Number has an illegal suffix\nfilename:6:18: Number has an illegal suffix\n"
    )


@pytest.mark.parse_error
def test_assignment_to_non_variable_expression():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn _start() {
    4 = 4+2;
    proc_exit(4u32)
}"""
    tools.expression_to_full_program(tools.get_doc_str())
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:5:5: Trying to assign to non-lvalue expression\n"
    )
