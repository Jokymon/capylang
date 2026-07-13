import tools
import pytest


@pytest.mark.good
def test_assigning_a_simple_string():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
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

@export
fn _start() {
    let s: string = "Some \\\" quote here";
    proc_exit(10u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 10


@pytest.mark.good
def test_string_with_escaped_unicode_char_in_ascii_range():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let s: string = "\\u{48}ello";
    proc_exit(*s.ptr as u32)
}"""
    # Hex 0x48 is the ASCII/Unicode code for 'H'
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 0x48


@pytest.mark.good
def test_string_with_escaped_unicode_char_in_2_byte_range():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let s: string = "\\u{4dc}";
    proc_exit(s.size)
}"""
    # Unicode 0x4dc is a cyrillic character that should be encodable in 2 bytes
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 2


@pytest.mark.good
def test_string_with_escaped_unicode_char_in_3_byte_range():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let s: string = "\\u{932}";
    proc_exit(s.size)
}"""
    # Unicode 0x932 is a devanagari character that should be encodable in 3 bytes
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 3


@pytest.mark.good
def test_strings_have_an_implict_ptr_field():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
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

@export
fn _start() {
    let s: string = "four";
    proc_exit(s.size)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 4


@pytest.mark.good
def test_string_size_field_has_type_u32_in_expressions():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let s: string = "four";
    proc_exit(s.size + 1u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 5


@pytest.mark.good
def test_string_ptr_field_has_type_u8_ptr():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let s: string = "four";
    let c: *u8 = s.ptr;
    proc_exit(s.size + 1u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 5


@pytest.mark.good
def test_creating_character_literals():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let c: char = 'a';
    proc_exit(3u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 3


@pytest.mark.good
def test_character_literals_can_be_utf8():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let c: char = '\\u{1234}';
    let e: u32 = (c as u32) >> 8u32;
    proc_exit(e)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 0x12


@pytest.mark.good
def test_strings_can_be_passed_as_arguments():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn len(s: string) -> u32
{
    s.size
}

@export
fn _start() {
    proc_exit(len("hello"))
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 5


# -------------------------------------------------------
# compile errors
@pytest.mark.parse_error
def test_unknown_string_fields_are_reported():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let s: string = "four";
    proc_exit(s.thing as u32)
}"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:7:15: Unknown record field 'thing'\n"
    )

# TODO: assigning from string literal should either create a copy of the string
# or it should only be allowed to assign to an immutable variable


@pytest.mark.parse_error
def test_unicode_escape_must_start_with_curly():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let s: string = "\\u932}";
    proc_exit(s.size)
}"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:6:24: Unicode escape sequence must start with '\\u{'\n"
    )
