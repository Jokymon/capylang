import pytest
import tools


@pytest.mark.good
def test_print_handles_empty_strings_and_64bit_boundaries():
    """
include "/lib/io.capy"

@export
fn _start() {
    if print("") != 0u32 { proc_exit(101u32); }
    if print("hello\n") != 0u32 { proc_exit(102u32); }
    if print_u64(18446744073709551615u64) != 0u32 { proc_exit(103u32); }
    print("\n");
    if print_s64(-9223372036854775808s64) != 0u32 { proc_exit(104u32); }
    print("\n");
    proc_exit(0u32)
}"""
    exit_code, stdout = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 0
    assert stdout == "hello\n18446744073709551615\n-9223372036854775808\n"


@pytest.mark.good
def test_write_bytes_returns_real_wasi_error_without_exiting():
    """
include "/lib/io.capy"

@export
fn _start() {
    let message: string = "hello";
    let error: u32 = write_bytes(4294967295u32, message.ptr, message.size);
    if error == 0u32 { proc_exit(101u32); }
    // The application chooses its own exit code after receiving the error.
    proc_exit(42u32)
}"""
    exit_code, stdout = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 42
    assert stdout == ""


@pytest.mark.good
def test_print_functions_return_errors_when_stdout_is_closed():
    """
include "/lib/io.capy"
import wasi_snapshot_preview1::fd_close(fd: u32) -> u32 as fd_close;

@export
fn _start() {
    // Close this test program's standard output to cause a real write error.
    if fd_close(1u32) != 0u32 { proc_exit(101u32); }

    let error: u32 = print("hello");
    if error == 0u32 { proc_exit(102u32); }
    if print_u64(123u64) != error { proc_exit(103u32); }
    if print_s64(123s64) != error { proc_exit(104u32); }
    if print_s64(-123s64) != error { proc_exit(105u32); }
    if print_u32(123u32) != error { proc_exit(106u32); }
    if print_s32(-123s32) != error { proc_exit(107u32); }

    proc_exit(42u32)
}"""
    exit_code, stdout = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 42
    assert stdout == ""
