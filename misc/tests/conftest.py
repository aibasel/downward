def pytest_addoption(parser):
    parser.addoption("--suppress", action="append", default=[],
                     help="Suppression files used in test-memory-leaks.py. These files must be located in the folder valgrind.")

def pytest_generate_tests(metafunc):
    if "suppression_files" in metafunc.fixturenames:
        tags = list(metafunc.config.option.suppress)
        metafunc.parametrize("suppression_files", [tags], scope='session')
