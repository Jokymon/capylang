import tools


def test_location_of_incomplete_expression():
    code = """f(4+)"""
    code = tools.expression_to_full_program(code)
    exit_code, stderr = tools.compile_test_code(code)

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:5:10: Expected a primary (function call, number, variable)\n"
    )
