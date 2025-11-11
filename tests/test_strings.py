import tools
import pytest


@pytest.mark.good
def test_assigning_a_simple_string():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn _start() {
    let s: string = "hello";
    proc_exit(10u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 10


@pytest.mark.good
def test_assigning_a_string_with_escaped_quotes():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn _start() {
    let s: string = "Some \\\" quote here";
    proc_exit(10u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 10


@pytest.mark.good
def test_strings_have_an_implict_ptr_field():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn _start() {
    let s: string = "Some \\\" quote here";
    let p: *u8 = s.ptr;
    proc_exit(10u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 10


@pytest.mark.good
def test_strings_have_an_implict_size_field():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn _start() {
    let s: string = "four";
    proc_exit(s.size as u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 4


@pytest.mark.good
def test_creating_character_literals():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn _start() {
    let c: char = 'a';
    proc_exit(3u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 3


# -------------------------------------------------------
# compile errors
@pytest.mark.parse_error
def test_unknown_string_fields_are_reported():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn _start() {
    let s: string = "four";
    proc_exit(s.thing as u32)
}"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:6:15: Unknown field 'thing' for string type\n"
    )

# TODO: assigning from string literal should either create a copy of the string
# or it should only be allowed to assign to an immutable variable