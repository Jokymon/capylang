import tools


def test_single_number():
    code = """$__imported_wasi_snapshot_preview1_proc_exit(4)"""
    code = tools.expression_to_full_program(code)
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 4


def test_single_number_with_type_suffix():
    code = """$__imported_wasi_snapshot_preview1_proc_exit(4u32)"""
    code = tools.expression_to_full_program(code)
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 4


def test_binary_addition():
    code = """$__imported_wasi_snapshot_preview1_proc_exit(5+3)"""
    code = tools.expression_to_full_program(code)
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 8


def test_binary_subtraction():
    code = """$__imported_wasi_snapshot_preview1_proc_exit(8-2)"""
    code = tools.expression_to_full_program(code)
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 6


def test_binary_int_division():
    code = """$__imported_wasi_snapshot_preview1_proc_exit(10/3)"""
    code = tools.expression_to_full_program(code)
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 3


def test_binary_modulus():
    code = """$__imported_wasi_snapshot_preview1_proc_exit(19%5)"""
    code = tools.expression_to_full_program(code)
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 4


def test_binary_multiplication():
    code = """$__imported_wasi_snapshot_preview1_proc_exit(3*5)"""
    code = tools.expression_to_full_program(code)
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 15


def test_multiple_operations():
    code = """$__imported_wasi_snapshot_preview1_proc_exit(5+ 2+3)"""
    code = tools.expression_to_full_program(code)
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 10


def test_simple_braced_expressions():
    code = """$__imported_wasi_snapshot_preview1_proc_exit((3+4))"""
    code = tools.expression_to_full_program(code)
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 7


def test_complex_braced_expressions():
    code = """$__imported_wasi_snapshot_preview1_proc_exit(3*(3+4))"""
    code = tools.expression_to_full_program(code)
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 21


def test_correct_priority_of_plus_and_multiply():
    code = """$__imported_wasi_snapshot_preview1_proc_exit(4*3+3)"""
    code = tools.expression_to_full_program(code)
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 15
