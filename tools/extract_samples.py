# This script extracts the capylang source code from all pytest-based tests,
# splits them up according to the pytest markers 'good' and 'parse_error' and
# puts according sources in the folders ./corpus/pass and ./corpus/fail
import pathlib
import pytest

parses_good = []
parses_bad = []


class CollectorPlugin:
    def pytest_collection_modifyitems(self, session, config, items):
        for item in items:
            if 'good' in item.keywords:
                parses_good.append(item)
            elif "parse_error" in item.keywords:
                parses_bad.append(item)


def list_tests(path="tests"):
    pytest.main([path, "--collect-only", "-q"],
                plugins=[CollectorPlugin()])


if __name__ == "__main__":
    corpus_dir = pathlib.Path("corpus")
    if not corpus_dir.exists():
        corpus_dir.mkdir()
    if not (corpus_dir / "pass").exists():
        (corpus_dir / "pass").mkdir()
    if not (corpus_dir / "fail").exists():
        (corpus_dir / "fail").mkdir()

    list_tests("tests")

    for t in parses_good:
        if t.function.__doc__ is None:
            continue
        with open(corpus_dir / "pass" / (t.function.__name__ + ".capy"), "w") as f:
            f.write(t.function.__doc__)
    for t in parses_bad:
        if t.function.__doc__ is None:
            continue
        with open(corpus_dir / "fail" / (t.function.__name__ + ".capy"), "w") as f:
            f.write(t.function.__doc__)
