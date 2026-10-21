import pytest
import tools


@pytest.mark.parse_error
def test_comparison_rejects_u64_and_s64():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let a: u64 = 5u64;
    let b: s64 = 5s64;
    let equal: bool = a == b;
    let not_equal: bool = a != b;
    let less: bool = a < b;
    let less_equal: bool = a <= b;
    let greater: bool = a > b;
    let greater_equal: bool = a >= b;
    proc_exit(0u32)
}"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert "Types for '=='-operation do not match" in stderr
    assert "Types for '!='-operation do not match" in stderr
    assert "Types for '<'-operation do not match" in stderr
    assert "Types for '<='-operation do not match" in stderr
    assert "Types for '>'-operation do not match" in stderr
    assert "Types for '>='-operation do not match" in stderr
    assert "they should be equal but are 'u64' and 's64'" in stderr


@pytest.mark.parse_error
def test_comparison_rejects_u32_and_s32():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let a: u32 = 5u32;
    let b: s32 = 5s32;
    let equal: bool = a == b;
    let not_equal: bool = a != b;
    let less: bool = a < b;
    let less_equal: bool = a <= b;
    let greater: bool = a > b;
    let greater_equal: bool = a >= b;
    proc_exit(0u32)
}"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert "Types for '=='-operation do not match" in stderr
    assert "Types for '!='-operation do not match" in stderr
    assert "Types for '<'-operation do not match" in stderr
    assert "Types for '<='-operation do not match" in stderr
    assert "Types for '>'-operation do not match" in stderr
    assert "Types for '>='-operation do not match" in stderr
    assert "they should be equal but are 'u32' and 's32'" in stderr


@pytest.mark.parse_error
def test_comparison_rejects_bool_and_u32():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let a: bool = true;
    let b: u32 = 1u32;
    let equal: bool = a == b;
    let not_equal: bool = a != b;
    let less: bool = a < b;
    let less_equal: bool = a <= b;
    let greater: bool = a > b;
    let greater_equal: bool = a >= b;
    proc_exit(0u32)
}"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert "Types for '=='-operation do not match" in stderr
    assert "Types for '!='-operation do not match" in stderr
    assert "Types for '<'-operation do not match" in stderr
    assert "Types for '<='-operation do not match" in stderr
    assert "Types for '>'-operation do not match" in stderr
    assert "Types for '>='-operation do not match" in stderr
    assert "they should be equal but are 'bool' and 'u32'" in stderr


@pytest.mark.good
def test_comparison_infers_u32_literal_type():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let value: u32 = 5u32;

    if value == 3 { proc_exit(1u32); }
    if (value != 3) == false { proc_exit(2u32); }
    if value < 3 { proc_exit(3u32); }
    if value <= 3 { proc_exit(4u32); }
    if (value > 3) == false { proc_exit(5u32); }
    if (value >= 3) == false { proc_exit(6u32); }

    proc_exit(0u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 0


@pytest.mark.good
def test_comparison_infers_u64_literal_type():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let value: u64 = 5u64;

    if value == 3 { proc_exit(1u32); }
    if (value != 3) == false { proc_exit(2u32); }
    if value < 3 { proc_exit(3u32); }
    if value <= 3 { proc_exit(4u32); }
    if (value > 3) == false { proc_exit(5u32); }
    if (value >= 3) == false { proc_exit(6u32); }

    proc_exit(0u32)
}"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 0
