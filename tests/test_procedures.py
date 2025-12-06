import pytest
import tools


@pytest.mark.good
def test_simple_procedure_definition():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    proc_exit(2u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 2


@pytest.mark.good
def test_function_without_parameters_with_return_type_is_usable():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn bla() -> u32 {
    (2 + 3) as u32
}

@export
fn _start() {
    proc_exit(bla())
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 5


@pytest.mark.good
def test_function_returning_pointer_type_works():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn bla() -> *u32 {
    100u32 as *u32
}

@export
fn _start() {
    let a: *u32 = bla();
    proc_exit(*a)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 0x42


@pytest.mark.good
def test_multiple_function_definitions():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn foo() {
    proc_exit(2u32)
}

@export
fn _start() {
    proc_exit(2u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 2


@pytest.mark.good
def test_function_definition_with_parameters():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn foo(a: s32, b: s32) -> s32 {
    4
}

@export
fn _start() {
    proc_exit(foo(1, 2) as u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 4


@pytest.mark.good
def test_function_arguments_are_usable():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn add2(a: u32) -> u32 {
    a + 2u32
}

@export
fn _start() {
    proc_exit(add2(3u32))
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 5


@pytest.mark.good
def test_functions_can_contain_multiple_expressions():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn exit_early(a: u32) {
    proc_exit(a)
}

@export
fn _start() {
    exit_early(34u32);
    proc_exit(3u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 34


@pytest.mark.good
def test_expressions_with_semicolon_are_void():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn add2(a: u32) -> u32 {
    a + 2u32
}

@export
fn _start() {
    // the following line should just be executed without
    // leaving a value on the stack
    add2(34u32);
    proc_exit(3u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 3


# ------------------------------------------------------------------
# import specific tests
@pytest.mark.skip(reason="There is a bug in proper alias handling")
def test_import_alias_name_can_be_used():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as pe;

@export
fn _start() {
    pe(3u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 3
