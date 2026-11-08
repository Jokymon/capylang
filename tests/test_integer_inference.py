"""Integer inference and s32 defaulting must finish before LIR emission."""

import re

import pytest
import tools


@pytest.mark.good
@pytest.mark.no_compile_matrix
@pytest.mark.parametrize(
    "body, expected_type",
    [
        pytest.param("let a = 42u64;", "u64", id="explicit-suffix"),
        pytest.param("let a: u64 = 42;", "u64", id="declaration"),
        pytest.param("let a = 42 + 1u64;", "u64", id="right-operand"),
        pytest.param("let a = 1u64 + 42;", "u64", id="left-operand"),
        pytest.param("take(42);", "u64", id="function-parameter"),
        pytest.param(
            "let mut a: u64 = 1u64; a = 42;", "u64", id="assignment"
        ),
        pytest.param(
            "let a = 42; let b: u64 = a + 1;", "u64", id="later-declaration"
        ),
        pytest.param(
            "let a = 42; let b = a; b == 42u64;",
            "u64", id="later-comparison-through-bindings",
        ),
        pytest.param("let a = 42;", "s32", id="default-literal"),
        pytest.param("42;", "s32", id="default-discarded-literal"),
        pytest.param("42 < 43;", "s32", id="default-comparison-operands"),
        pytest.param("let a = 42 + 43;", "s32", id="default-expression"),
        pytest.param(
            "let a = 42; let b = a;", "s32", id="default-through-bindings"
        ),
        pytest.param(
            "let a = 42; let b = 1u64;", "s32", id="unrelated-u64-is-not-context"
        ),
    ],
)
def test_integer_types_are_concrete_before_emission(body, expected_type):
    source = f"""
fn take(value: u64) {{ }}

@export
fn _start() {{
    {body}
}}
"""
    lir = tools.compile_to_lir(source)

    # LIR numbers carry resolved types. Checking here catches a backend that
    # silently chooses i32 while leaving the language type unknown.
    number_types = re.findall(
        r"_type: Number\s+value: (-?\d+)\s+assigned_type: ([^\n]+)", lir
    )
    for literal in ["42", "43"] if "43" in body else ["42"]:
        inferred_types = [typ for value, typ in number_types if value == literal]
        assert inferred_types, f"Literal {literal} missing from LIR:\n{lir}"
        assert all(typ == expected_type for typ in inferred_types), inferred_types
    # Symbol references may print a resolved type variable's internal ID.
    # Only the actual LIR value types must be concrete.
    assert not re.search(
        r"^\s*assigned_type: (?:type_var\(|!unassigned)", lir, re.MULTILINE
    ), lir


@pytest.mark.good
@pytest.mark.parametrize(
    "operator, expected",
    [("+", 44), ("-", 40), ("*", 84), ("/", 21), ("%", 0), ("<<", 168), (">>", 10)],
)
def test_arithmetic_result_type_propagates_to_operands(operator, expected):
    source = f"""
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {{
    let a = 42;
    let b: u64 = a {operator} 2;
    proc_exit((b != {expected}u64) as u32)
}}
"""
    exit_code, _ = tools.run_test_code(source)

    assert exit_code == 0


@pytest.mark.good
@pytest.mark.no_compile_matrix
@pytest.mark.parametrize("literal", ["-2147483648", "-1", "0", "2147483647"])
def test_default_s32_accepts_boundary_values(literal):
    source = f"""
@export
fn _start() {{
    let value = {literal};
}}
"""
    lir = tools.compile_to_lir(source)

    assert re.search(
        rf"_type: Number\s+value: {literal}\s+assigned_type: s32\b", lir
    ), lir


@pytest.mark.good
@pytest.mark.parametrize(
    "return_expression",
    [
        pytest.param("return 3000000000;", id="explicit"),
        pytest.param("3000000000", id="implicit"),
        pytest.param("let a = 2999999958; return a + 42;", id="explicit-expression"),
        pytest.param("let a = 2999999958; a + 42", id="implicit-expression"),
        pytest.param("if (true) { return 3000000000; } 3000000000", id="nested-return"),
    ],
)
def test_return_type_provides_context_before_defaulting(return_expression):
    source = f"""
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn value() -> u64 {{
    {return_expression}
}}

@export
fn _start() {{
    proc_exit((value() - 2999999958u64) as u32)
}}
"""
    exit_code, _ = tools.run_test_code(source)

    assert exit_code == 42


@pytest.mark.good
def test_return_type_context_is_local_to_each_function():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn wide() -> u64 {
    return 3000000000;
}

fn signed() -> s64 {
    let value = -9;
    return value / 2;
}

fn nothing() {
    return;
}

@export
fn _start() {
    nothing();
    let result: u64 = wide() - 2999999958u64;
    proc_exit((result as u32) + ((signed() + 4s64) as u32))
}
"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 42


@pytest.mark.parse_error
@pytest.mark.no_compile_matrix
@pytest.mark.parametrize("body", ["return 42u32;", "42u32"])
def test_return_type_context_does_not_override_explicit_literal_type(body):
    source = f"""
fn value() -> u64 {{
    {body}
}}
"""
    exit_code, stderr = tools.compile_test_code(source)

    assert exit_code == 1, stderr
    assert "u32" in stderr
    assert "u64" in stderr


@pytest.mark.parse_error
@pytest.mark.no_compile_matrix
@pytest.mark.parametrize("body", ["return 42;", "42"])
def test_return_type_context_rejects_integer_literals_for_bool(body):
    source = f"""
fn value() -> bool {{
    {body}
}}
"""
    exit_code, stderr = tools.compile_test_code(source)

    assert exit_code == 1, stderr
    assert "Numeric literal '42' requires an integer type, but has type 'bool'" in stderr


@pytest.mark.parse_error
@pytest.mark.no_compile_matrix
@pytest.mark.parametrize("body", ["return 256;", "256"])
def test_return_type_context_enforces_literal_range(body):
    source = f"""
fn value() -> u8 {{
    {body}
}}
"""
    exit_code, stderr = tools.compile_test_code(source)

    assert exit_code == 1, stderr
    assert "Numeric literal '256' exceeds valid range for u8 (0..255)" in stderr


@pytest.mark.parse_error
@pytest.mark.no_compile_matrix
@pytest.mark.parametrize("body", ["return;", "42u64;"])
def test_return_type_context_still_requires_a_returned_value(body):
    source = f"""
fn value() -> u64 {{
    {body}
}}
"""
    exit_code, stderr = tools.compile_test_code(source)

    assert exit_code == 1, stderr
    assert "void" in stderr
    assert "u64" in stderr


@pytest.mark.good
def test_large_unsuffixed_integer_uses_u64_declaration_context():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let value: u64 = 3000000000;
    proc_exit((value - 2999999958u64) as u32)
}
"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 42


@pytest.mark.good
def test_default_s32_operations_are_signed():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let value = -9;
    let quotient = value / 2;
    let negative: bool = quotient < 0;
    proc_exit(((quotient + 10) as u32) + (negative as u32))
}
"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    # Signed division gives -4; unsigned interpretation would give another result.
    assert exit_code == 7


@pytest.mark.parse_error
@pytest.mark.no_compile_matrix
@pytest.mark.parametrize(
    "literal", ["2147483648", "-2147483649", "3000000000", "-3000000000"]
)
def test_default_s32_rejects_out_of_range_literals(literal):
    source = f"""
@export
fn _start() {{
    let value = {literal};
}}
"""
    exit_code, stderr = tools.compile_test_code(source)

    assert exit_code == 1, stderr
    assert (
        f"Numeric literal '{literal}' exceeds valid range for s32 "
        "(-2147483648..2147483647)"
    ) in stderr


@pytest.mark.parse_error
@pytest.mark.no_compile_matrix
@pytest.mark.parametrize(
    "type_name, literal, bounds",
    [("u8", "256", "0..255"), ("s8", "-129", "-128..127")],
)
def test_inferred_integer_type_controls_literal_range(type_name, literal, bounds):
    source = f"""
@export
fn _start() {{
    let value: {type_name} = {literal};
}}
"""
    exit_code, stderr = tools.compile_test_code(source)

    assert exit_code == 1, stderr
    assert (
        f"Numeric literal '{literal}' exceeds valid range for {type_name} ({bounds})"
    ) in stderr


@pytest.mark.parse_error
@pytest.mark.no_compile_matrix
@pytest.mark.parametrize(
    "statement",
    ["let value: u32 = 42u64;", "let value = 42u64 + 1u32;"],
)
def test_defaulting_does_not_hide_explicit_type_conflicts(statement):
    source = f"""
@export
fn _start() {{
    {statement}
}}
"""
    exit_code, stderr = tools.compile_test_code(source)

    assert exit_code == 1, stderr
    assert "u32" in stderr
    assert "u64" in stderr
