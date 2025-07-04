import tools


def test_comments_after_code_are_ignored():
    code = """
fn $_start() {
    $__imported_wasi_snapshot_preview1_proc_exit(4) // some comment
}"""
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 4


def test_complete_lines_of_comment_are_ignored():
    code = """// Initial line
fn $_start() {
    $__imported_wasi_snapshot_preview1_proc_exit(3+1)
}"""
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 4


def test_multiple_lines_of_comments_are_ignored():
    code = """// Line 1
// Line 2
fn $_start() {
    $__imported_wasi_snapshot_preview1_proc_exit(3+1)
}"""
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 4


def test_multiple_lines_of_comments_with_empties_are_ignored():
    code = """// Line 1

// Line 2
fn $_start() {
    $__imported_wasi_snapshot_preview1_proc_exit(3+1)
}"""
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 4
