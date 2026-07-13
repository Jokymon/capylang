import tools
import pytest


@pytest.mark.good
def test_simple_u32_field_access():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

record s {
    field1: u32,
};

@export
fn _start() {
    let v: *s = allocate s{
        field1=18u32,
    };
    proc_exit(v.field1)
}
"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 18


@pytest.mark.good
def test_multiple_fields_are_correctly_initialised():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

record s {
    field1: u32,
    field2: u32,
};

@export
fn _start() {
    let v: *s = allocate s{
        field1=20u32,
        field2=15u32,
    };
    proc_exit(v.field1 - v.field2)
}
"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 5


@pytest.mark.good
def test_allocate_uses_memory_based_storage():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

record s {
    field1: u32,
};

@export
fn _start() {
    let v: *s = allocate s{
        field1=18u32,
    };
    // This is a hacky approach to overwrite the record field through
    // memory. If we still get 18 as return value, then the record was
    // probably lowered to local variables
    let x: *u32 = v as *u32;
    *x = 99u32;
    proc_exit(v.field1)
}
"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 99


@pytest.mark.good
def test_nested_record_access():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

record s2 {
    field1: u32,
};

record s1 {
    sub: *s2,
};

@export
fn _start() {
    let v2: *s2 = allocate s2{
        field1=18u32,
    };
    let v1: *s1 = allocate s1{
        sub=v2,
    };
    proc_exit(v1.sub.field1)
}
"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 18


# ######## FAILURE HANDLING ###########
@pytest.mark.parse_error
def test_failure_illegal_deref_syntax():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

record s {
    field1: u32,
};

@export
fn _start() {
    let v: *s = allocate s{
        field1=18u32,
    };
    proc_exit(v.)
}
"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:13:17: Missing field name after '.'\n"
    )


@pytest.mark.parse_error
def test_failure_dereferencing_non_record_type():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

@export
fn _start() {
    let v: u32 = 32u32;
    proc_exit(v.field1)
}
"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:7:15: Dereferencing non-record variable or field 'v'\nfilename:7:5: Function 'proc_exit' expects signature (u32); called with signature (!unassigned)\n"
    )


@pytest.mark.parse_error
def test_failure_unknown_variable_in_initialisation():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

record s {
    field1: u32,
};

@export
fn _start() {
    let v: *s = allocate s{
        field1=w,
    };
    proc_exit(v.field1)
}
"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:11:16: Undefined variable: 'w'\n"
    )


@pytest.mark.parse_error
def test_failure_invalid_dereferencing_in_field_init():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

record s {
    field1: u32,
};

@export
fn _start() {
    let w: u32 = 3u32;
    let v: *s = allocate s{
        field1=w.a,
    };
    proc_exit(v.field1)
}
"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:12:16: Dereferencing non-record variable or field 'w'\n"
    )


@pytest.mark.parse_error
def test_failure_accessing_unknown_field_in_dereferencing():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

record s {
    field1: u32,
};

@export
fn _start() {
    let v: *s = allocate s{
        field1=18u32,
    };
    proc_exit(v.fld)
}
"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:13:15: Unknown record field 'fld'\nfilename:13:5: Function 'proc_exit' expects signature (u32); called with signature (!unassigned)\n"
    )


@pytest.mark.parse_error
def test_failure_accessing_unknown_field_in_initialisation():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

record s {
    field1: u32,
};

@export
fn _start() {
    let v: *s = allocate s{
        field1=12u32,
        fld=18u32,
    };
    proc_exit(v.field1)
}
"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:12:9: Unknown record field 'fld'\n"
    )


@pytest.mark.parse_error
def test_failure_missing_field_in_initialisation():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

record s {
    field1: u32,
};

@export
fn _start() {
    let v: *s = allocate s{
    };
    proc_exit(v.field1)
}
"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:10:26: Record field 'field1' not initialised\n"
    )


@pytest.mark.parse_error
def test_failure_missing_and_unknown_fields_in_initialisation():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

record s {
    field1: u32,
    field2: u32,
};

@export
fn _start() {
    let v: *s = allocate s{
        field1=12u32,
        extra=18u32,
    };
    proc_exit(v.field1)
}
"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:11:26: Record field 'field2' not initialised\nfilename:13:9: Unknown record field 'extra'\n"
    )
