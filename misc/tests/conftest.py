# This file is read by pytest before running the tests to set them up.
# We use it to add the option '--suppress' to the tests that is used by
# memory leak tests to ignore compiler-specific false positives.

def pytest_addoption(parser):
    parser.addoption("--suppress", action="append", default=[],
                     help="Suppression files used in test-memory-leaks.py.")

def pytest_generate_tests(metafunc):
    if "suppression_files" in metafunc.fixturenames:
        supression_files = list(metafunc.config.option.suppress)
        metafunc.parametrize("suppression_files", [supression_files], scope='session')
