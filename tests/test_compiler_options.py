import tools
import pytest


@pytest.mark.good
def test_setting_min_blocks_increases_memory_size():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    proc_exit(memory_size())
}
    """
    exit_code, _ = tools.run_test_code(
        tools.get_doc_str(),
        options={"mem-min-blocks": 4})

    assert exit_code == 4


@pytest.mark.good
@pytest.mark.skip("Currently have no runtime that can easily provide imported memories")
def test_compile_with_imported_memory():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let addr: *u8 = 108u32 as *u8;
    proc_exit(*addr as u32)
}"""
    exit_code, _ = tools.run_test_code(
        tools.get_doc_str(),
        options={"memory": "imported"})

    assert exit_code == 0x10


