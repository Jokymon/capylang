import tools
import pytest


@pytest.mark.good
def test_simple_u32_field_access():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

record s {
    field1: u32,
};

fn _start() {
    let v: s = s{
        field1=18,
    };
    proc_exit(v.field1)
}
"""
    exit_code, _ = tools.run_test_code(tools.get_doc_str())

    assert exit_code == 18


@pytest.mark.good
def test_nested_record_access():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

record s2 {
    field1: u32,
};

record s1 {
    sub: s2,
};

fn _start() {
    let v2: s2 = s2{
        field1=18,
    };
    let v1: s1 = s1{
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

fn _start() {
    let v: s = s{
        field1=18,
    };
    proc_exit(v.)
}
"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:12:17: Missing field name after '.'\n"
    )


@pytest.mark.parse_error
def test_failure_dereferencing_non_record_type():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

fn _start() {
    let v: u32 = 32;
    proc_exit(v.field1)
}
"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:6:15: Dereferencing non-record variable or field 'v'\nfilename:6:15: Unknown record field 'field1'\nfilename:6:5: Function 'proc_exit' expects signature (u32); called with signature (!unassigned)\n"
    )


@pytest.mark.parse_error
def test_failure_unknown_variable_in_initialisation():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

record s {
    field1: u32,
};

fn _start() {
    let v: s = s{
        field1=w,
    };
    proc_exit(v.field1)
}
"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:10:16: Undefined variable: 'w'\n"
    )


@pytest.mark.parse_error
def test_failure_invalid_dereferencing_in_field_init():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

record s {
    field1: u32,
};

fn _start() {
    let w: u32 = 3u32;
    let v: s = s{
        field1=w.a,
    };
    proc_exit(v.field1)
}
"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:11:16: Dereferencing non-record variable or field 'w'\nfilename:11:16: Unknown record field 'a'\n"
    )


@pytest.mark.parse_error
def test_failure_accessing_unknown_field_in_dereferencing():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

record s {
    field1: u32,
};

fn _start() {
    let v: s = s{
        field1=18,
    };
    proc_exit(v.fld)
}
"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:12:15: Unknown record field 'fld'\nfilename:12:5: Function 'proc_exit' expects signature (u32); called with signature (!unassigned)\n"
    )


@pytest.mark.parse_error
def test_failure_accessing_unknown_field_in_initialisation():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

record s {
    field1: u32,
};

fn _start() {
    let v: s = s{
        field1=12,
        fld=18,
    };
    proc_exit(v.field1)
}
"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:11:9: Unknown record field 'fld'\n"
    )


@pytest.mark.parse_error
def test_failure_missing_field_in_initialisation():
    """
import wasi_snapshot_preview1::proc_exit(exit_code: u32) as proc_exit;

record s {
    field1: u32,
};

fn _start() {
    let v: s = s{
    };
    proc_exit(v.field1)
}
"""
    exit_code, stderr = tools.compile_test_code(tools.get_doc_str())

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:9:16: Record field 'field1' not initialised\n"
    )
