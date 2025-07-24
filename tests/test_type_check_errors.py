import tools


def test_mismatching_types_in_binary_op():
    code = """proc_exit(4s32 + 5u32)"""
    code = tools.expression_to_full_program(code)
    exit_code, stderr = tools.compile_test_code(code)

    assert exit_code == 1
    assert (
        tools.normalize_filename_from_output(stderr)
        == "filename:5:21: Types for '+'-operation do not match; they should be equal but are 's32' and 'u32'\n"
    )
