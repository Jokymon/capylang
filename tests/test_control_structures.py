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


# ----------------------------------------
# semantic errors
def test_then_and_else_branch_need_identical_types():
    code = """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn _start() {
    let condition: bool = true;
    let result: u32 = if condition {
        10u32
    } else {
        12s32
    }
    proc_exit(result)
}
"""
    exit_code, stderr = tools.compile_test_code(code)

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:6:23: 'then' and 'else' branches have mismatching types 'u32' and 's32'\n"
    )


def test_then_with_return_with_empty_else_is_invalid():
    code = """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn _start() {
    let condition: bool = true;
    let result: u32 = if condition {
        10u32
    }
    proc_exit(result)
}
"""
    exit_code, stderr = tools.compile_test_code(code)

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:6:23: 'if' with return type is missing an 'else' branch\n"
    )
