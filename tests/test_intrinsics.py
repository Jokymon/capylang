import tools
import pytest


@pytest.mark.good
def test_memory_size_intrinsic():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    proc_exit(memory_size())
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 2


@pytest.mark.good
def test_memory_grow_intrinsic():
    # TODO: a missing ; after memory_grow() would be incorrectly accepted by the
    # handwritten parser but is correctly refused by the ANTLR grammar
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let old_size: s32 = memory_grow(4);
    if (old_size==-1) {
        // This shouldn't happen and is just used to better identify bigger
        // problems; growing the memory should normally succeed and then return
        // the old size, return -1 means, allocated failed
        proc_exit(99);
    }
    proc_exit(memory_size() - old_size as u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 4


@pytest.mark.good
def test_memory_fill_intrinsic():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    memory_fill(0x2000u32, 23u32, 10u32);

    let value_ptr: *u8 = 0x2000u32 as *u8;
    proc_exit(*value_ptr as u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 23
