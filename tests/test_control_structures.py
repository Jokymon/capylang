import tools


def test_then_body_is_executed_with_true_condition():
    code = """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn _start() {
    let condition: bool = true;
    if condition {
        proc_exit(10u32)
    }
    proc_exit(4u32)
}"""
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 10


def test_else_body_is_executed_with_false_condition():
    code = """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn _start() {
    let condition: bool = false;
    if condition {
        proc_exit(10u32)
    } else {
        proc_exit(8u32)
    }
    proc_exit(4u32)
}"""
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 8


def test_then_body_is_not_executed_with_false_condition():
    code = """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn _start() {
    let condition: bool = false;
    if condition {
        proc_exit(10u32)
    }
    proc_exit(4u32)
}"""
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 4


def test_if_is_an_expression_with_return():
    code = """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn _start() {
    let condition: bool = true;
    let result: u32 = if condition {
        10u32
    } else {
        12u32
    }
    proc_exit(result)
}"""
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 10
