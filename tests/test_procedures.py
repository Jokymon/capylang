import tools


def test_simple_procedure_definition():
    code = """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn _start() {
    proc_exit(2)
}"""
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 2


def test_function_without_parameters_with_return_type_is_usable():
    code = """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn bla() -> s32 {
    2 + 3
}

fn _start() {
    proc_exit(bla())
}"""
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 5


def test_multiple_function_definitions():
    code = """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn foo() {
    proc_exit(2)
}

fn _start() {
    proc_exit(2)
}"""
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 2


def test_function_definition_with_parameters():
    code = """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn foo(a: s32, b: s32) -> s32 {
    4
}

fn _start() {
    proc_exit(foo(1, 2))
}"""
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 4
