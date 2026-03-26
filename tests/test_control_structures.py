import tools
import pytest


@pytest.mark.good
def test_then_body_is_executed_with_true_condition():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let condition: bool = true;
    if condition {
        proc_exit(10u32)
    }
    proc_exit(4u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 10


@pytest.mark.good
def test_else_body_is_executed_with_false_condition():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let condition: bool = false;
    if condition {
        proc_exit(10u32)
    } else {
        proc_exit(8u32)
    }
    proc_exit(4u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 8


@pytest.mark.good
def test_then_body_is_not_executed_with_false_condition():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let condition: bool = false;
    if condition {
        proc_exit(10u32)
    }
    proc_exit(4u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 4


@pytest.mark.good
def test_assignment_on_last_line_in_if_works():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let condition: bool = true;
    let mut value: u32 = 8u32;

    if condition {
        value = 9u32;
    }
    proc_exit(value)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 9


@pytest.mark.good
def test_assignment_to_deref_pointer_on_last_line_in_if_works():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let condition: bool = true;
    let mut value: *u32 = 100u32 as *u32;
    *value = 8u32;

    if condition {
        *value = 9u32;
    }
    proc_exit(*value)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 9


@pytest.mark.good
def test_if_is_an_expression_with_return():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let condition: bool = true;
    let result: u32 = if condition {
        10u32
    } else {
        12u32
    }
    proc_exit(result)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 10


@pytest.mark.good
def test_while_loop():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let mut condition: bool = true;
    let mut value: u32 = 10u32;

    while condition {
        value = value + 1u32;
        condition = false;
    }

    proc_exit(value)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 11


@pytest.mark.good
def test_break_in_a_while_loop_works():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let mut value: u32 = 0u32;

    while value < 30 {
        if value==12 {
            break;
        }
        value = value + 1;
    }

    proc_exit(value)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 12


@pytest.mark.good
def test_assignment_on_last_line_in_while_works():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let condition: bool = false;
    let mut value: u32 = 8u32;

    while condition {
        value = 9u32;
    }
    proc_exit(value)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 8


@pytest.mark.good
def test_assignment_to_deref_pointer_on_last_line_in_while_works():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let condition: bool = false;
    let mut value: *u32 = 100u32 as *u32;
    *value = 8u32;

    while condition {
        *value = 9u32;
    }
    proc_exit(*value)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 8


@pytest.mark.good
def test_functions_can_be_exited_early():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

global mut g: u32=10u32;

fn f() {
    let a: u32 = 20;
    return;
    g = a;
}

@export
fn _start() {
    f();

    proc_exit(g)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 10


@pytest.mark.good
def test_returned_value_in_functions_is_used():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn f() -> u32 {
    return 5u32 + 8u32;
    42u32
}

@export
fn _start() {
    proc_exit(f())
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 13


# ----------------------------------------
# semantic errors
def test_then_and_else_branch_need_identical_types():
    code = """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
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
        == "filename:7:23: 'then' and 'else' branches have mismatching types 'u32' and 's32'\n"
    )


def test_then_with_return_with_empty_else_is_invalid():
    code = """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
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
        == "filename:7:23: 'if' with return type is missing an 'else' branch\n"
    )


@pytest.mark.parse_error
def test_nested_if_branch_type_mismatch_is_reported():
    code = """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let condition: bool = true;
    let result: u32 = if condition {
        if condition {
            10u32
        } else {
            11u32
        }
    } else {
        12s32
    }
    proc_exit(result)
}
"""
    exit_code, stderr = tools.compile_test_code(code)
    normalized = tools.normalize_filename_from_output(stderr)

    assert exit_code == 1
    assert "'then' and 'else' branches have mismatching types 'u32' and 's32'" in normalized


@pytest.mark.parse_error
def test_returns_with_mismatched_types_is_wrong():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn f(value: bool) -> u32 {
    if (value) {
        return 10u32;
    }
    else {
        return 5s32;
    }
    // TODO: currently we still need to explicitly create a returned value in
    // the last line of the function, we are not yet clever enough to figure out
    // that all control paths in the if already exit the function and we always
    // have a return value
    0u32
}

@export
fn _start() {
    proc_exit(f(false))
}"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:9:16: Returned expression has type 's32' but function expects 'u32'\n"
    )
