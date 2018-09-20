# -*- coding: utf-8 -*-

from __future__ import print_function

import sys

"""
We document Fast Downward exit codes at
http://www.fast-downward.org/ExitCodes. Please update this documentation when
making changes below.
"""

SUCCESS = 0
SEARCH_PLAN_FOUND_AND_OUT_OF_MEMORY = 1
SEARCH_PLAN_FOUND_AND_OUT_OF_TIME = 2
SEARCH_PLAN_FOUND_AND_OUT_OF_MEMORY_AND_TIME = 3

TRANSLATE_UNSOLVABLE = 10
SEARCH_UNSOLVABLE = 11
SEARCH_UNSOLVED_INCOMPLETE = 12

TRANSLATE_OUT_OF_MEMORY = 20
TRANSLATE_OUT_OF_TIME = 21
SEARCH_OUT_OF_MEMORY = 22
SEARCH_OUT_OF_TIME = 23
SEARCH_OUT_OF_MEMORY_AND_TIME = 24

TRANSLATE_CRITICAL_ERROR = 30
TRANSLATE_INPUT_ERROR = 31
SEARCH_CRITICAL_ERROR = 32
SEARCH_INPUT_ERROR = 33
SEARCH_UNSUPPORTED = 34
DRIVER_CRITICAL_ERROR = 35
DRIVER_INPUT_ERROR = 36
DRIVER_UNSUPPORTED = 37


def print_stderr(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)


def is_unrecoverable(exitcode):
    # Exit codes in the range from 30 to 39 represent unrecoverable failures.
    return 30 <= exitcode < 40


def exit_with_driver_critical_error(msg):
    print_stderr(msg)
    sys.exit(DRIVER_CRITICAL_ERROR)


def exit_with_driver_input_error(msg):
    print_stderr(msg)
    sys.exit(DRIVER_INPUT_ERROR)


def exit_with_driver_unsupported_error(msg):
    print_stderr(msg)
    sys.exit(DRIVER_UNSUPPORTED)


def generate_portfolio_exitcode(exitcodes):
    """A portfolio's exitcode is determined as follows:

    There is exactly one type of unexpected exit code -> use it.
    There are multiple types of unexpected exit codes -> SEARCH_CRITICAL_ERROR.
    [..., SUCCESS, ...] -> SUCCESS
    [..., SEARCH_UNSOLVABLE, ...] -> SEARCH_UNSOLVABLE
    [..., SEARCH_UNSOLVED_INCOMPLETE, ...] -> SEARCH_UNSOLVED_INCOMPLETE
    [..., SEARCH_OUT_OF_MEMORY, ..., SEARCH_OUT_OF_TIME, ...] -> SEARCH_OUT_OF_MEMORY_AND_TIME
    [..., SEARCH_OUT_OF_TIME, ...] -> SEARCH_OUT_OF_TIME
    [..., SEARCH_OUT_OF_MEMORY, ...] -> SEARCH_OUT_OF_MEMORY
    """
    print("Exit codes: {}".format(exitcodes))
    exitcodes = set(exitcodes)
    unrecoverable_codes = [code for code in exitcodes if is_unrecoverable(code)]

    # There are unrecoverable exit codes.
    if unrecoverable_codes:
        print("Error: Unexpected exit codes: {}".format(unrecoverable_codes))
        if len(unrecoverable_codes) == 1:
            return (unrecoverable_codes[0], False)
        else:
            return (SEARCH_CRITICAL_ERROR, False)

    # At least one plan was found.
    if SUCCESS in exitcodes:
        if SEARCH_OUT_OF_MEMORY in exitcodes and SEARCH_OUT_OF_TIME in exitcodes:
            return (SEARCH_PLAN_FOUND_AND_OUT_OF_MEMORY_AND_TIME, True)
        elif SEARCH_OUT_OF_MEMORY in exitcodes:
            return (SEARCH_PLAN_FOUND_AND_OUT_OF_MEMORY, True)
        elif SEARCH_OUT_OF_TIME in exitcodes:
            return (SEARCH_PLAN_FOUND_AND_OUT_OF_TIME, True)
        else:
            return (SUCCESS, True)

    # A config proved unsolvability or did not find a plan.
    for code in [SEARCH_UNSOLVABLE, SEARCH_UNSOLVED_INCOMPLETE]:
        if code in exitcodes:
            return (code, False)

    # No plan was found due to hitting resource limits.
    if SEARCH_OUT_OF_MEMORY in exitcodes and SEARCH_OUT_OF_TIME in exitcodes:
        return (SEARCH_OUT_OF_MEMORY_AND_TIME, False)
    elif SEARCH_OUT_OF_MEMORY in exitcodes:
        return (SEARCH_OUT_OF_MEMORY, False)
    elif SEARCH_OUT_OF_TIME in exitcodes:
        return (SEARCH_OUT_OF_TIME, False)

    assert False, "Error: Unhandled exit codes: {}".format(exitcodes)
