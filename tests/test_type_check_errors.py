import tools
import pytest


@pytest.mark.parse_error
def test_mismatching_types_in_binary_op():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    proc_exit(4s32 + 5u32)
}"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:6:21: Types for '+'-operation do not match; they should be equal but are 's32' and 'u32'\nfilename:6:5: Function 'proc_exit' expects signature (u32); called with signature (!unassigned)\n"
    )
