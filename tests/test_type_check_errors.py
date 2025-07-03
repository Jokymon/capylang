import tools


def normalize_filename_from_output(stdxxx):
    print(stdxxx)
    if stdxxx.lower().startswith("c:/"):
        stdxxx = stdxxx[3:]

    idx = stdxxx.find(":")
    if idx == -1:
        return stdxxx
    else:
        return "filename" + stdxxx[idx:]


def test_mismatching_types_in_binary_op():
    code = """f(4s32 + 5u32)"""
    code = tools.expression_to_full_program(code)
    exit_code, stderr = tools.compile_test_code(code)

    assert exit_code == 1
    assert (
        normalize_filename_from_output(stderr)
        == "filename:3:13: Types for '+'-operation do not match; they should be equal but are 's32' and 'u32'\n"
    )
