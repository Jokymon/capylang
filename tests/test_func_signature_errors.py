import tools


def normalize_filename_from_output(stdxxx):
    print(stdxxx)
    if stdxxx.lower().startswith("c:/"):
        stdxxx = stdxxx[3:]

    idx = stdxxx.find(":")
    if idx == -1:
        return stdxxx
    else:
        return "filename" + stdxxx[idx:]


def test_undefined_function():
    code = """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn _start() {
    proc_exit(add2(10, 20))
}
"""
    exit_code, stderr = tools.compile_test_code(code)

    assert exit_code == 1
    assert (
        normalize_filename_from_output(stderr)
        == "filename:5:27: Function 'add2' is not defined\n"
    )


def test_mismatching_argument_count():
    code = """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn add2(a: u32) -> u32 {
    a + 2u32
}

fn _start() {
    proc_exit(add2(10, 20))
}
"""
    exit_code, stderr = tools.compile_test_code(code)

    assert exit_code == 1
    assert (
        normalize_filename_from_output(stderr)
        == "filename:9:20: Function 'add2' expects signature (u32); called with signature (s32, s32)\n"
    )
