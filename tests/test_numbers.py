import tools
import pytest


@pytest.mark.good
def test_negative_numbers_can_be_assigned():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let n: s32 = -12s32;
    proc_exit(17u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 17


@pytest.mark.good
def test_negative_numbers_are_correclty_encoded():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let a: s32 = 50s32;
    let b: s32 = -12s32;
    proc_exit((a+b) as u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 38


@pytest.mark.good
def test_double_negatives_are_supported():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let a: s32 = 50s32;
    proc_exit((a - -15s32) as u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 65
