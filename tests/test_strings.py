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
