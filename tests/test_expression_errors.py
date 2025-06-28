import tools


def normalize_filename_from_output(stdxxx):
    if stdxxx.lower().startswith("c:/"):
        stdxxx = stdxxx[3:]

    idx = stdxxx.find(":")
    if idx == -1:
        return stdxxx
    else:
        return "filename" + stdxxx[idx:]


def test_number_with_illegal_suffix():
    code = """$__imported_wasi_snapshot_preview1_proc_exit(4xy)"""
    code = tools.expression_to_full_program(code)
    exit_code, stderr = tools.compile_test_code(code)

    assert exit_code == 1
    assert (
        normalize_filename_from_output(stderr)
        == "filename:3:53: Number has an illegal suffix\n"
    )
