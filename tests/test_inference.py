import tools
import pytest


@pytest.mark.good
def test_type_is_infered_from_call_site():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let a = 10;
    proc_exit(a)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 10


@pytest.mark.good
def test_numeric_type_is_infered_from_addition_expression():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let mut a: u32 = 10;
    a = a + 2;
    proc_exit(a)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 12
