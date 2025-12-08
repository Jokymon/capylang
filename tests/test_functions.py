import tools
import pytest


@pytest.mark.good
def test_function_body_can_be_empty():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn bla() { }

@export
fn _start() {
    bla();
    proc_exit(4u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 4


@pytest.mark.good
def test_function_arguments_have_their_own_scope():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

// To check that we have separte scopes, we define two functions taking a parameter 'a'
// but each of them being a different type. Then we have to use that parameter in the
// function according to the parameter type and not get any errors from semantic checks.
fn foo(a: u32) -> u32 {
    a + 1u32
}

fn bar(a: s32) -> s32 {
    a + 1s32
}

@export
fn _start() {
    foo(2u32);
    bar(5s32);
    proc_exit(4u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 4


@pytest.mark.good
def test_functions_can_have_simple_attributes():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn bla() { }

@exp
@export
fn _start() {
    bla();
    proc_exit(4u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 4


@pytest.mark.good
def test_functions_can_have_complex_attributes():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@imp(ns="wasi_snapshort_preview1", name="proc_exit")
fn pce(exit_code: u32)
{ }

@export
fn _start() {
    proc_exit(4u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 4


# ----------------------------------------
# syntactic errors
@pytest.mark.parse_error
def test_missing_curly_brace_at_function_definition_start():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start()
    proc_exit(10)
}
"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:5:12: Expecting an opening brace '{' for function body definition\n"
    )


@pytest.mark.parse_error
def test_missing_curly_brace_at_function_definition_end():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    proc_exit(10)
"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:6:18: Expecting a closing brace '}' to terminate the body\n"
    )


@pytest.mark.parse_error
def test_missing_namespace_identifier_in_import():
    """
import wasi_snapshot_preview1 proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    proc_exit(10)
}
"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:2:30: Namespace is expected to be followed by '::' and a symbol name\n"
    )


@pytest.mark.parse_error
def test_missing_colon_after_namespace_in_import():
    """
import ::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    proc_exit(10)
}
"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:2:7: Expecting a namespace identifier\n"
    )
