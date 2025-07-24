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
