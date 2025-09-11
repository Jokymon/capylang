import tools


def test_let_statement_with_simple_type():
    code = """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn _start() {
    let a: u32 = 2u32;
    proc_exit(a)
}"""
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 2


def test_pointer_dereferencing():
    # For this test, the data in the linear memory is hardcoded in the
    # compiled WAT file. That data starts at address 100
    code = """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn _start() {
    let a: *u32 = 104u32;
    proc_exit(*a)
}"""
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 0x10
