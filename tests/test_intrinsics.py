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


