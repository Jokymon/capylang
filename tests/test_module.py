import tools
import pytest


@pytest.mark.good
def test_include_works():
    """
include "/lib/wasi.capy"

@export
fn _start() {
    proc_exit(7u32)
}
    """
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 7


@pytest.mark.parse_error
def test_include_of_missing_file_fails():
    """
include "/missing/file.capy"
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    proc_exit(7u32)
}
    """
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:2:9: Could not open include file '/missing/file.capy'\n"
    )

@pytest.mark.parse_error
def test_failure_module_parsing_eof():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    proc_exit(2u32)
}
{
"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:7:2: Unexpected trailing code after function definition\n"
    )
