import tools
import pytest


@pytest.mark.good
def test_cabi_realloc_returnes_aligned_address():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let a: *u32 = cabi_realloc(0, 0, 4, 3) as *u32;
    let b: *u32 = cabi_realloc(0, 0, 4, 2) as *u32;

    // b is created after we allocate a with a size of 3, but
    // we allocate b with alignment 4, so the address should be
    // divisible by 4
    proc_exit((b as u32) % 4u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 0
