import tools


def test_single_number():
    code = """4"""
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 4


def test_binary_addition():
    code = """5+3"""
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 8


def test_binary_multiplication():
    code = """3*5"""
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 15


def test_multiple_operations():
    code = """5+ 2+3"""
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 10


def test_simple_braced_expressions():
    code = """(3+4)"""
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 7


def test_complex_braced_expressions():
    code = """3*(3+4)"""
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 21