import tools
import pytest


def test_single_number_with_type_suffix():
    code = """proc_exit(4u32)"""
    code = tools.expression_to_full_program(code)
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 4


def test_conversion_operator():
    code = """proc_exit(4s32 as u32)"""
    code = tools.expression_to_full_program(code)
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 4


def test_conversion_of_braced_expression():
    code = """proc_exit((4s32 + 3s32) as u32)"""
    code = tools.expression_to_full_program(code)
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 7


def test_binary_addition():
    code = """proc_exit(5u32+3u32)"""
    code = tools.expression_to_full_program(code)
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 8


def test_binary_subtraction():
    code = """proc_exit(8u32-2u32)"""
    code = tools.expression_to_full_program(code)
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 6


def test_binary_int_division():
    code = """proc_exit((10/3) as u32)"""
    code = tools.expression_to_full_program(code)
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 3


def test_binary_modulus():
    code = """proc_exit((19%5) as u32)"""
    code = tools.expression_to_full_program(code)
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 4


def test_binary_multiplication():
    code = """proc_exit((3*5) as u32)"""
    code = tools.expression_to_full_program(code)
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 15


def test_multiple_operations():
    code = """proc_exit((5+ 2+3) as u32)"""
    code = tools.expression_to_full_program(code)
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 10


def test_simple_braced_expressions():
    code = """proc_exit(((3+4)) as u32)"""
    code = tools.expression_to_full_program(code)
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 7


def test_complex_braced_expressions():
    code = """proc_exit((3*(3+4)) as u32)"""
    code = tools.expression_to_full_program(code)
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 21


def test_correct_priority_of_plus_and_multiply():
    code = """proc_exit((4*3+3) as u32)"""
    code = tools.expression_to_full_program(code)
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 15


@pytest.mark.good
def test_u8_deref_on_rhs():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn _start() {
    let addr: *u8 = 108u32 as *u8;
    proc_exit(*addr as u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 0x10


def test_u8_deref_on_lhs():
    code = """let addr1: *u8 = 108u32 as *u8;
    let addr2: *u8 = 109u32 as *u8;
    *addr2 = 40u8;
    // the following assignment shouldn't change addr2 because
    // it is only changing a u8
    *addr1 = 11;
proc_exit(*addr2 as u32)"""
    code = tools.expression_to_full_program(code)
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 40


@pytest.mark.good
def test_variables_can_be_modified():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn _start() {
    let mut a: u32 = 10u32;
    a = 20u32;
    proc_exit(a)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 20
