import tools


# ----------------------------------------
# syntactic errors
def test_missing_curly_brace_at_function_definition_start():
    code = """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn _start()
    proc_exit(10)
}
"""
    exit_code, stderr = tools.compile_test_code(code)

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:4:12: Expecting an opening brace '{' for function body definition\n"
    )


def test_missing_curly_brace_at_function_definition_end():
    code = """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn _start() {
    proc_exit(10)
"""
    exit_code, stderr = tools.compile_test_code(code)

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:5:18: Expecting a closing brace '}' after function body definition\n"
    )


def test_missing_namespace_identifier_in_import():
    code = """
import wasi_snapshot_preview1 proc_exit(exit_code: u32) as proc_exit;

fn _start() {
    proc_exit(10)
}
"""
    exit_code, stderr = tools.compile_test_code(code)

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:2:30: Namespace is expected to be followed by '::' and a symbol name\n"
    )


def test_missing_colon_after_namespace_in_import():
    code = """
import ::proc_exit(exit_code: u32) as proc_exit;

fn _start() {
    proc_exit(10)
}
"""
    exit_code, stderr = tools.compile_test_code(code)

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:2:7: Expecting a namespace identifier\n"
    )
