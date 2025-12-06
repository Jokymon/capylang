import tools
import pytest


@pytest.mark.parse_error
def test_number_with_illegal_suffix():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    proc_exit(4xy)
}"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:6:18: Number has an illegal suffix\n"
    )


@pytest.mark.parse_error
def test_multiple_numbers_with_illegal_suffix():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let a: u32 = 5ab;
    proc_exit(4xy+a)
}"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:6:21: Number has an illegal suffix\nfilename:7:18: Number has an illegal suffix\n"
    )


@pytest.mark.parse_error
def test_assignment_to_non_variable_expression():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    4 = 4+2;
    proc_exit(4u32)
}"""
    tools.expression_to_full_program(tools.get_doc_str())
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:6:5: Trying to assign to non-lvalue expression\n"
    )


@pytest.mark.parse_error
def test_immutable_variables_cant_be_modified():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let a: u32 = 10u32;
    a = 20u32;
    proc_exit(a)
}"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:7:5: Can't assign to immutable variable 'a'\n"
    )
