import tools
import pytest

# Tests for the 64-bit integer types u64 and s64.


@pytest.mark.good
def test_u64_arithmetic_beyond_32bit():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let a: u64 = 5000000000u64;
    let b: u64 = a * 2u64 + 42u64;
    proc_exit((b - 10000000000u64) as u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 42


@pytest.mark.good
def test_s64_division_and_comparison_are_signed():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let a: s64 = -9s64;
    let q: s64 = a / 2s64;
    let negative: bool = a < 0s64;
    proc_exit(((q + 10s64) as u32) + (negative as u32))
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    # -9 / 2 = -4, so 6 + 1
    assert exit_code == 7


@pytest.mark.good
def test_u64_division_and_comparison_are_unsigned():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let a: u64 = 18446744073709551615u64;
    let half: u64 = a / 2u64;
    let bigger: bool = a > half;
    proc_exit(((half - 9223372036854775800u64) as u32) + (bigger as u32))
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    # half = 9223372036854775807, so 7 + 1
    assert exit_code == 8


@pytest.mark.good
def test_u64_in_functions_and_globals():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

global mut total: u64 = 4294967296u64;

fn add(a: u64, b: u64) -> u64 {
    a + b
}

@export
fn _start() {
    total = add(total, 5u64);
    proc_exit((total - 4294967296u64) as u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 5


@pytest.mark.good
def test_casts_from_32bit_to_64bit():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let a: s32 = -5s32;
    let b: u32 = 4000000000u32;
    let wide_a: s64 = a as s64;
    let wide_b: u64 = b as u64;
    proc_exit(((wide_a + 10s64) as u32) + ((wide_b - 3999999990u64) as u32))
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    # sign-extension keeps -5 (-5 + 10 = 5), zero-extension keeps 4000000000 (10)
    assert exit_code == 15


@pytest.mark.parse_error
def test_comparison_of_32bit_and_64bit_is_rejected():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let a: u64 = 5u64;
    let b: bool = a == 5u32;
    proc_exit(0u32)
}"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())
    normalized = tools.normalize_filename_from_output(stderr)

    assert exit_code == 1
    assert "do not match; they should be equal but are 'u64' and 'u32'" in normalized
