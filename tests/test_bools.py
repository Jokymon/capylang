import tools
import pytest


@pytest.mark.good
def test_assigning_bool():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let b: bool = true;
    proc_exit(30u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 30


@pytest.mark.good
def test_a_boolean_true_is_a_1():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let b: bool = true;
    proc_exit(b as u32 + 5u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 6


@pytest.mark.good
def test_a_boolean_false_is_a_0():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let b: bool = false;
    proc_exit(b as u32 + 5u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 5


@pytest.mark.good
def test_equality_check_creates_a_bool():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let a: u32 = 10u32;
    let b: bool = (a == 10u32);
    proc_exit(b as u32 + 5u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 6


@pytest.mark.good
def test_inequality_check_creates_a_bool():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let a: u32 = 10u32;
    let b: bool = (a != 10u32);
    proc_exit(b as u32 + 5u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 5


@pytest.mark.good
def test_logical_and_is_a_bool():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let a: u32 = 10u32;
    let b: u32 = 11u32;
    let c: bool = (a != 8u32 && b != 8u32);
    proc_exit(c as u32 + 5u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 6


@pytest.mark.good
def test_logical_or_is_a_bool():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let a: u32 = 10u32;
    let b: u32 = 11u32;
    let c: bool = (a != 10u32 || b != 8u32);
    proc_exit(c as u32 + 5u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 6
