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
    4u32 = 4u32+2u32;
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


@pytest.mark.parse_error
def test_undefined_identifiers_do_not_share_state():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let a: u32 = missing_a;
    let b: u32 = missing_b;
    proc_exit(0u32)
}"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())
    normalized = tools.normalize_filename_from_output(stderr)

    assert exit_code == 1
    assert "Undefined variable: 'missing_a'" in normalized
    assert "Undefined variable: 'missing_b'" in normalized


# --------------------------------------------------------------------
# numeric literal range errors
@pytest.mark.parse_error
def test_valid_range_for_u8_is_checked():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let a: u8 = 500u8;
    proc_exit(a as u32)
}"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:6:17: Numeric literal '500' exceeds valid range for u8 (0..255)\n"
    )


@pytest.mark.parse_error
def test_valid_range_for_u16_is_checked():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let a: u16 = 80000u16;
    proc_exit(a as u32)
}"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:6:18: Numeric literal '80000' exceeds valid range for u16 (0..65535)\n"
    )


@pytest.mark.parse_error
def test_valid_range_for_u32_is_checked():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let a: u32 = 5000000000u32;
    proc_exit(a as u32)
}"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:6:18: Numeric literal '5000000000' exceeds valid range for u32 (0..4294967295)\n"
    )


@pytest.mark.parse_error
def test_valid_range_for_s8_is_checked():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let a: s8 = 200s8;
    proc_exit(a as u32)
}"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:6:17: Numeric literal '200' exceeds valid range for s8 (-128..127)\n"
    )


@pytest.mark.parse_error
def test_valid_range_for_s16_is_checked():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let a: s16 = 50000s16;
    proc_exit(a as u32)
}"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:6:18: Numeric literal '50000' exceeds valid range for s16 (-32768..32767)\n"
    )


@pytest.mark.parse_error
def test_valid_range_for_s32_is_checked():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let a: s32 = 3000000000s32;
    proc_exit(a as u32)
}"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:6:18: Numeric literal '3000000000' exceeds valid range for s32 (-2147483648..2147483647)\n"
    )
