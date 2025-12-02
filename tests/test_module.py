import tools
import pytest


@pytest.mark.good
def test_failure_module_parsing_eof():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

export fn _start() {
    proc_exit(2u32)
}
{
"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:6:2: Unexpected trailing code after function definition\n"
    )
