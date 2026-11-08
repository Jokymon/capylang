import tools
import pytest


@pytest.mark.parse_error
@pytest.mark.no_compile_matrix
@pytest.mark.parametrize(
    "prefix, body, target_type, value_type",
    [
        pytest.param("", "let mut a: u64 = 1u64; a = 2u32;", "u64", "u32", id="width"),
        pytest.param("", "let mut a: s32 = 1s32; a = 2u32;", "s32", "u32", id="signedness"),
        pytest.param(
            "", "let mut a: u64 = 1u64; let b: u32 = 2u32; a = b;",
            "u64", "u32", id="variable-value",
        ),
        pytest.param("", "let mut a: u64; a = 2u32;", "u64", "u32", id="uninitialised-target"),
        pytest.param("global mut a: u64 = 1u64;", "a = 2u32;", "u64", "u32", id="global"),
        pytest.param(
            "", "let p: *u64 = 512u32 as *u64; *p = 2u32;",
            "u64", "u32", id="pointer-dereference",
        ),
        pytest.param(
            "", "let mut p: *u32 = 512u32 as *u32; p = 512u32 as *u64;",
            "u32*", "u64*", id="pointer-value",
        ),
        pytest.param(
            "", "let p: *u8 = 512u32 as *u8; *p = 11u32 as *u8;",
            "u8", "u8*", id="pointer-to-integer",
        ),
        pytest.param("", "let mut a: bool = false; a = 1u32;", "bool", "u32", id="bool"),
        pytest.param(
            "fn nothing() { }", "let mut a: u64 = 1u64; a = nothing();",
            "u64", "void", id="void-value",
        ),
    ],
)
def test_incompatible_assignment_types_are_rejected(prefix, body, target_type, value_type):
    source = f"""
{prefix}

@export
fn _start() {{
    {body}
}}
"""
    exit_code, stderr = tools.compile_test_code(source)

    assert exit_code == 1, stderr
    assert (
        f"Cannot assign value of type '{value_type}' to target of type '{target_type}'"
    ) in stderr


@pytest.mark.parse_error
def test_variable_cannot_be_initialized_with_a_void_function_call():
    """
fn do_nothing() { }

@export
fn _start() {
    let a = do_nothing();
}"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert "Cannot initialize a variable with an expression that returns no value" in stderr


@pytest.mark.parse_error
def test_mismatching_types_in_binary_op():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    proc_exit(4s32 + 5u32)
}"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:6:21: Types for '+'-operation do not match; they should be equal but are 's32' and 'u32'\nfilename:6:5: Function 'proc_exit' expects signature (u32); called with signature (!unassigned)\n"
    )
