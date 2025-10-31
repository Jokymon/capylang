import tools


def test_assigning_a_simple_string():
    code = """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn _start() {
    let s: string = "hello";
    proc_exit(10u32)
}"""
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 10


def test_assigning_a_string_with_escaped_quotes():
    code = """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn _start() {
    let s: string = "Some \\\" quote here";
    proc_exit(10u32)
}"""
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 10


def test_strings_have_an_implict_ptr_field():
    code = """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn _start() {
    let s: string = "Some \\\" quote here";
    let p: *u32 = s.ptr;
    proc_exit(10u32)
}"""
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 10


def test_strings_have_an_implict_size_field():
    code = """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn _start() {
    let s: string = "four";
    proc_exit(s.size as u32)
}"""
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 4


# -------------------------------------------------------
# compile errors
def test_unknown_string_fields_are_reported():
    code = """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn _start() {
    let s: string = "four";
    proc_exit(s.thing as u32)
}"""
    exit_code, stderr = tools.compile_test_code(code)

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:6:15: Unknown field 'thing' for string type\n"
    )
