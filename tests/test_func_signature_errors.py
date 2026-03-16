import tools
import pytest

# ----------------------------------------
# syntactic errors


@pytest.mark.parse_error
def test_missing_colon_for_parameter_types():
    """
import wasi_snapshot_preview1::proc_exit(exit_code u32) as proc_exit;

@export
fn _start() {
    proc_exit(10u32)
}
"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:2:51: Expecting a colon ':' between parameter name and parameter type\n"
    )


@pytest.mark.parse_error
def test_missing_open_bracket_for_parameters():
    """
import wasi_snapshot_preview1::proc_exit exit_code: u32) as proc_exit;

@export
fn _start() {
    proc_exit(10u32)
}
"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:2:41: Expecting an opening bracket '(' for function parameters\n"
    )


@pytest.mark.parse_error
def test_missing_closing_bracket_for_parameters():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32 as proc_exit;

@export
fn _start() {
    proc_exit(10u32)
}
"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:2:56: Expecting an closing bracket ')' for function parameters\n"
    )


@pytest.mark.parse_error
def test_missing_function_identifier():
    """
import wasi_snapshot_preview1:: (exit_code: u32) as proc_exit;

@export
fn _start() {
    proc_exit(10u32)
}
"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:2:32: Expecting a function name\n"
    )


# ----------------------------------------
# semantic errors


@pytest.mark.parse_error
def test_undefined_function():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    proc_exit(add2(10u32, 20u32))
}
"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:6:15: Function 'add2' is not defined\n"
    )


@pytest.mark.parse_error
def test_mismatching_argument_count():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn add2(a: u32) -> u32 {
    a + 2u32
}

@export
fn _start() {
    proc_exit(add2(10s32, 20s32))
}
"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:10:15: Function 'add2' expects signature (u32); called with signature (s32, s32)\n"
    )


@pytest.mark.parse_error
def test_mismatching_return_types():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn const_u32() -> u32 {
    34s32
}

@export
fn _start() {
    proc_exit(const_u32())
}
"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:5:5: Returned value of type s32 doesn't match the declared return type u32\n"
    )
