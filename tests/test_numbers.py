import tools
import pytest


@pytest.mark.good
def test_negative_numbers_can_be_assigned():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let n: s32 = -12s32;
    proc_exit(17u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 17


@pytest.mark.good
def test_hexadecimal_numbers_are_allowed():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let n: u32 = 0x2bu32;
    proc_exit(n)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 43


@pytest.mark.good
def test_negative_numbers_are_correclty_encoded():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let a: s32 = 50s32;
    let b: s32 = -12s32;
    proc_exit((a+b) as u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 38


@pytest.mark.good
def test_double_negatives_are_supported():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let a: s32 = 50s32;
    proc_exit((a - -15s32) as u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 65


@pytest.mark.good
def test_variable_type_is_deduced_from_numeric_literal():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let a = 51u32;
    proc_exit(a)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 51


@pytest.mark.good
def test_literal_type_is_deduced_from_variable_type():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let a: u32 = 51;
    proc_exit(a)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 51


@pytest.mark.good
def test_u8_boundary_values_are_accepted():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let lo: u8 = 0u8;
    let hi: u8 = 255u8;
    proc_exit(((hi as u32) - (lo as u32)) - 255u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 0


@pytest.mark.good
def test_s8_boundary_values_are_accepted():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let lo: s8 = -128s8;
    let hi: s8 = 127s8;
    proc_exit((lo + hi + 1s8) as u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 0


@pytest.mark.parse_error
def test_u8_just_outside_upper_bound_is_rejected():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let v: u8 = 256u8;
    proc_exit(v as u32)
}"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())
    normalized = tools.normalize_filename_from_output(stderr)

    assert exit_code == 1
    assert "Numeric literal '256' exceeds valid range for u8 (0..255)" in normalized


@pytest.mark.parse_error
def test_s8_just_outside_lower_bound_is_rejected():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let v: s8 = -129s8;
    proc_exit(v as u32)
}"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())
    normalized = tools.normalize_filename_from_output(stderr)

    assert exit_code == 1
    assert "Numeric literal '-129' exceeds valid range for s8 (-128..127)" in normalized


@pytest.mark.good
def test_u16_boundary_values_are_accepted():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let lo: u16 = 0u16;
    let hi: u16 = 65535u16;
    proc_exit(((hi as u32) - (lo as u32)) - 65535u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 0


@pytest.mark.good
def test_s16_boundary_values_are_accepted():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let lo: s16 = -32768s16;
    let hi: s16 = 32767s16;
    proc_exit((lo + hi + 1s16) as u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 0


@pytest.mark.good
def test_u32_boundary_values_are_accepted():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let lo: u32 = 0u32;
    let hi: u32 = 4294967295u32;
    proc_exit((hi - 4294967295u32) + lo)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 0


@pytest.mark.good
def test_s32_boundary_values_are_accepted():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let lo: s32 = -2147483648s32;
    let hi: s32 = 2147483647s32;
    proc_exit((lo + hi + 1s32) as u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 0


@pytest.mark.parse_error
def test_u16_just_outside_upper_bound_is_rejected():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let v: u16 = 65536u16;
    proc_exit(v as u32)
}"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())
    normalized = tools.normalize_filename_from_output(stderr)

    assert exit_code == 1
    assert "Numeric literal '65536' exceeds valid range for u16 (0..65535)" in normalized


@pytest.mark.parse_error
def test_s16_just_outside_lower_bound_is_rejected():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let v: s16 = -32769s16;
    proc_exit(v as u32)
}"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())
    normalized = tools.normalize_filename_from_output(stderr)

    assert exit_code == 1
    assert "Numeric literal '-32769' exceeds valid range for s16 (-32768..32767)" in normalized


@pytest.mark.parse_error
def test_u32_just_outside_upper_bound_is_rejected():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let v: u32 = 4294967296u32;
    proc_exit(v as u32)
}"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())
    normalized = tools.normalize_filename_from_output(stderr)

    assert exit_code == 1
    assert "Numeric literal '4294967296' exceeds valid range for u32 (0..4294967295)" in normalized


@pytest.mark.parse_error
def test_s32_just_outside_upper_bound_is_rejected():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let v: s32 = 2147483648s32;
    proc_exit(v as u32)
}"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())
    normalized = tools.normalize_filename_from_output(stderr)

    assert exit_code == 1
    assert "Numeric literal '2147483648' exceeds valid range for s32 (-2147483648..2147483647)" in normalized
