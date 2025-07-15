import tools


def test_single_number():
    code = """proc_exit(4)"""
    code = tools.expression_to_full_program(code)
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 4


def test_single_number_with_type_suffix():
    code = """proc_exit(4u32)"""
    code = tools.expression_to_full_program(code)
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 4


def test_conversion_operator():
    code = """proc_exit(4u32 as s32)"""
    code = tools.expression_to_full_program(code)
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 4


def test_conversion_of_braced_expression():
    code = """proc_exit((4u32 + 3u32) as s32)"""
    code = tools.expression_to_full_program(code)
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 7


def test_binary_addition():
    code = """proc_exit(5+3)"""
    code = tools.expression_to_full_program(code)
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 8


def test_binary_subtraction():
    code = """proc_exit(8-2)"""
    code = tools.expression_to_full_program(code)
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 6


def test_binary_int_division():
    code = """proc_exit(10/3)"""
    code = tools.expression_to_full_program(code)
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 3


def test_binary_modulus():
    code = """proc_exit(19%5)"""
    code = tools.expression_to_full_program(code)
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 4


def test_binary_multiplication():
    code = """proc_exit(3*5)"""
    code = tools.expression_to_full_program(code)
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 15


def test_multiple_operations():
    code = """proc_exit(5+ 2+3)"""
    code = tools.expression_to_full_program(code)
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 10


def test_simple_braced_expressions():
    code = """proc_exit((3+4))"""
    code = tools.expression_to_full_program(code)
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 7


def test_complex_braced_expressions():
    code = """proc_exit(3*(3+4))"""
    code = tools.expression_to_full_program(code)
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 21


def test_correct_priority_of_plus_and_multiply():
    code = """proc_exit(4*3+3)"""
    code = tools.expression_to_full_program(code)
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 15
