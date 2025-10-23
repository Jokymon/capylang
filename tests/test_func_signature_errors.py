import tools

# ----------------------------------------
# syntactic errors


def test_missing_colon_for_parameter_types():
    code = """
import wasi_snapshot_preview1::proc_exit(exit_code u32) as proc_exit;

fn _start() {
    proc_exit(10)
}
"""
    exit_code, stderr = tools.compile_test_code(code)

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:2:51: Expecting a colon ':' between parameter name and parameter type\n"
    )


def test_missing_open_bracket_for_parameters():
    code = """
import wasi_snapshot_preview1::proc_exit exit_code: u32) as proc_exit;

fn _start() {
    proc_exit(10)
}
"""
    exit_code, stderr = tools.compile_test_code(code)

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:2:41: Expecting an opening bracket '(' for function parameters\n"
    )


def test_missing_closing_bracket_for_parameters():
    code = """
import wasi_snapshot_preview1::proc_exit(exit_code: u32 as proc_exit;

fn _start() {
    proc_exit(10)
}
"""
    exit_code, stderr = tools.compile_test_code(code)

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:2:56: Expecting an closing bracket ')' for function parameters\n"
    )


def test_missing_function_identifier():
    code = """
import wasi_snapshot_preview1:: (exit_code: u32) as proc_exit;

fn _start() {
    proc_exit(10)
}
"""
    exit_code, stderr = tools.compile_test_code(code)

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:2:32: Expecting a function name\nfilename:5:18: Function 'proc_exit' is not defined\n"
    )


# ----------------------------------------
# semantic errors


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
        tools.normalize_filename_from_output(stderr)
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
        tools.normalize_filename_from_output(stderr)
        == "filename:9:15: Function 'add2' expects signature (u32); called with signature (s32, s32)\n"
    )


def test_mismatching_return_types():
    code = """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn const_u32() -> u32 {
    34
}

fn _start() {
    proc_exit(const_u32())
}
"""
    exit_code, stderr = tools.compile_test_code(code)

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:5:5: Returned value of type s32 doesn't match the declared return type u32\n"
    )
