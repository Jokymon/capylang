import tools
import pytest


@pytest.mark.good
def test_comments_after_code_are_ignored():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn _start() {
    proc_exit(4u32) // some comment
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 4


@pytest.mark.good
def test_complete_lines_of_comment_are_ignored():
    """// Initial line
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn _start() {
    proc_exit(3u32+1u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 4


@pytest.mark.good
def test_multiple_lines_of_comments_are_ignored():
    """import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;
// Line 1
// Line 2
fn _start() {
    proc_exit(3u32+1u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 4


@pytest.mark.good
def test_multiple_lines_of_comments_with_empties_are_ignored():
    """// Line 1
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

// Line 2
fn _start() {
    proc_exit(3u32+1u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 4
