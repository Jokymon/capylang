import tools


def test_assigning_bool():
    code = """let b: bool = true;\nproc_exit(30u32)"""
    code = tools.expression_to_full_program(code)
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 30


def test_a_boolean_true_is_a_1():
    code = """let b: bool = true;\nproc_exit(b as u32 + 5u32)"""
    code = tools.expression_to_full_program(code)
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 6


def test_a_boolean_false_is_a_0():
    code = """let b: bool = false;\nproc_exit(b as u32 + 5u32)"""
    code = tools.expression_to_full_program(code)
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 5
