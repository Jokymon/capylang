import tools


def test_simple_procedure_definition():
    code = """
fn $_start() {
    $__imported_wasi_snapshot_preview1_proc_exit(2)
}"""
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 2
