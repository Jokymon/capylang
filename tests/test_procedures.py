import tools


def test_simple_procedure_definition():
    code = """
fn _start() {
    __imported_wasi_snapshot_preview1_proc_exit(2)
}"""
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 2


def test_function_without_parameters_with_return_type_is_usable():
    code = """
fn bla() -> s32 {
    2 + 3
}

fn _start() {
    __imported_wasi_snapshot_preview1_proc_exit(bla())
}"""
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 5


def test_multiple_function_definitions():
    code = """
fn foo() {
    __imported_wasi_snapshot_preview1_proc_exit(2)
}

fn _start() {
    __imported_wasi_snapshot_preview1_proc_exit(2)
}"""
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 2


def test_function_definition_with_parameters():
    code = """
fn foo(a: s32, b: s32) -> s32 {
    4
}

fn _start() {
    __imported_wasi_snapshot_preview1_proc_exit(foo(1, 2))
}"""
    exit_code, _ = tools.run_test_code(code)

    assert exit_code == 4
