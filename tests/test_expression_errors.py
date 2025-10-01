import tools


def test_number_with_illegal_suffix():
    code = """proc_exit(4xy)"""
    code = tools.expression_to_full_program(code)
    exit_code, stderr = tools.compile_test_code(code)

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:5:18: Number has an illegal suffix\n"
    )


def test_multiple_numbers_with_illegal_suffix():
    code = """let a: u32 = 5ab;
proc_exit(4xy+a)"""
    code = tools.expression_to_full_program(code)
    exit_code, stderr = tools.compile_test_code(code)

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:5:21: Number has an illegal suffix\nfilename:6:14: Number has an illegal suffix\n"
    )


def test_assignment_to_non_variable_expression():
    code = """4 = 4+2;
proc_exit(4u32)"""
    code = tools.expression_to_full_program(code)
    exit_code, stderr = tools.compile_test_code(code)

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:5:5: Trying to assign to non-lvalue expression\n"
    )
