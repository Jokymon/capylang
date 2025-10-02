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


def test_pointer_deref_on_lhs():
    code = """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn _start() {
    let a: *u32 = 100u32;
    *a = 45u32;
    proc_exit(*a)
}"""
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 45


def test_non_pointer_deref_on_lhs():
    code = """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn _start() {
    let a: u32 = 100u32;
    a = 45u32;
    proc_exit(a)
}"""
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 45


# ######## FAILURE HANDLING ###########
def test_missing_type_spec_for_let():
    code = """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn _start() {
    let a: = 100u32;
    a = 45u32;
    proc_exit(a)
}"""
    exit_code, stderr = tools.compile_test_code(code)

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:5:11: Expecting an identifier for the type specification\n"
    )
