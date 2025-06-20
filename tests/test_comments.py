import tools


def test_comments_after_code_are_ignored():
    code = """4 // some comment"""
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 4


def test_complete_lines_of_comment_are_ignored():
    code = """// Initial line
3+1"""
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 4