import tools
import pytest


@pytest.mark.good
def test_function_body_can_be_empty():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn bla() { }

fn _start() {
    bla();
    proc_exit(4u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 4


@pytest.mark.good
def test_function_can_be_exported():
    # This test doesn't check that the 'export'ed function is actually exported
    # in the resulting WASM. It only makes sure, the 'export' keyword is
    # accepted
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

export fn _start() {
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

fn _start()
    proc_exit(10)
}
"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:4:12: Expecting an opening brace '{' for function body definition\n"
    )


@pytest.mark.parse_error
def test_missing_curly_brace_at_function_definition_end():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn _start() {
    proc_exit(10)
"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:5:18: Expecting a closing brace '}' to terminate the body\n"
    )


@pytest.mark.parse_error
def test_missing_namespace_identifier_in_import():
    """
import wasi_snapshot_preview1 proc_exit(exit_code: u32) as proc_exit;

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


@pytest.mark.parse_error
def test_export_not_followed_by_fn_is_nicely_explained():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

export _start() {
    proc_exit(10u32)
}
"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:4:7: Expecting keyword 'fn' after 'export'\n"
    )
