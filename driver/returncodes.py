# -*- coding: utf-8 -*-

from __future__ import print_function

import signal


EXIT_PLAN_FOUND = 0
EXIT_CRITICAL_ERROR = 1
EXIT_INPUT_ERROR = 2
EXIT_UNSUPPORTED = 3
EXIT_UNSOLVABLE = 4
EXIT_UNSOLVED_INCOMPLETE = 5
EXIT_OUT_OF_MEMORY = 6
EXIT_TIMEOUT = 7
EXIT_TIMEOUT_AND_MEMORY = 8
EXIT_SIGXCPU = -signal.SIGXCPU if hasattr(signal, "SIGXCPU") else None

EXPECTED_EXITCODES = set([
    EXIT_PLAN_FOUND, EXIT_UNSOLVABLE, EXIT_UNSOLVED_INCOMPLETE,
    EXIT_OUT_OF_MEMORY, EXIT_TIMEOUT, EXIT_SIGXCPU])


def generate_portfolio_exitcode(exitcodes):
    """A portfolio's exitcode is determined as follows:

    There is exactly one type of unexpected exit code -> use it.
    There are multiple types of unexpected exit codes -> EXIT_CRITICAL_ERROR.
    [..., EXIT_PLAN_FOUND, ...] -> EXIT_PLAN_FOUND
    [..., EXIT_UNSOLVABLE, ...] -> EXIT_UNSOLVABLE
    [..., EXIT_UNSOLVED_INCOMPLETE, ...] -> EXIT_UNSOLVED_INCOMPLETE
    [..., EXIT_OUT_OF_MEMORY, ..., EXIT_TIMEOUT, ...] -> EXIT_TIMEOUT_AND_MEMORY
    [..., EXIT_TIMEOUT, ...] -> EXIT_TIMEOUT
    [..., EXIT_OUT_OF_MEMORY, ...] -> EXIT_OUT_OF_MEMORY
    """
    print("Exit codes: %s" % exitcodes)
    exitcodes = set(exitcodes)
    if EXIT_SIGXCPU in exitcodes:
        exitcodes.remove(EXIT_SIGXCPU)
        exitcodes.add(EXIT_TIMEOUT)
    unexpected_codes = exitcodes - EXPECTED_EXITCODES
    if unexpected_codes:
        print("Error: Unexpected exit codes: %s" % list(unexpected_codes))
        if len(unexpected_codes) == 1:
            return unexpected_codes.pop()
        else:
            return EXIT_CRITICAL_ERROR
    for code in [EXIT_PLAN_FOUND, EXIT_UNSOLVABLE, EXIT_UNSOLVED_INCOMPLETE]:
        if code in exitcodes:
            return code
    for code in [EXIT_OUT_OF_MEMORY, EXIT_TIMEOUT]:
        if exitcodes == set([code]):
            return code
    if exitcodes == set([EXIT_OUT_OF_MEMORY, EXIT_TIMEOUT]):
        return EXIT_TIMEOUT_AND_MEMORY
    print("Error: Unhandled exit codes: %s" % exitcodes)
    return EXIT_CRITICAL_ERROR
