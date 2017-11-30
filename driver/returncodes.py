# -*- coding: utf-8 -*-

from __future__ import print_function

import signal

## Exit codes denoting successful execution allowing to continue with other
## components: translator completed and/or a plan was found

EXIT_SUCCESS = 0 # translator completed, or search found a plan, or validate validated a plan.
EXIT_SEARCH_PLAN_FOUND_AND_OUT_OF_MEMORY = 1 # only for portfolios
EXIT_SEARCH_PLAN_FOUND_AND_OUT_OF_TIME = 2 # only for portfolios
EXIT_SEARCH_PLAN_FOUND_AND_OUT_OF_MEMORY_AND_TIME = 3 # only for portfolios

## Exit codes denoting no plan was found.

EXIT_TRANSLATE_UNSOLVABLE = 10
EXIT_SEARCH_UNSOLVABLE = 11
EXIT_SEARCH_UNSOLVED_INCOMPLETE = 12

## Exit codes denoting "expected failures" such as running out of memory or
## time.

EXIT_TRANSLATE_OUT_OF_MEMORY = 20
EXIT_TRANSLATE_OUT_OF_TIME = 21 # not used at the moment because we use SIGXCPU instead if applicable
EXIT_TRANSLATE_SIGXCPU = 256-signal.SIGXCPU if hasattr(signal, "SIGXCPU") else None
EXIT_SEARCH_OUT_OF_MEMORY = 22
EXIT_SEARCH_OUT_OF_TIME = 23 # currently only used by anytime configurations because with use SIGXCPU for single searches if applicable
EXIT_SEARCH_SIGXCPU = -signal.SIGXCPU if hasattr(signal, "SIGXCPU") else None
EXIT_SEARCH_OUT_OF_MEMORY_AND_TIME = 24 # only for anytime search configurations

## Exit codes denoting unrecoverable errors

EXIT_TRANSLATE_CRITICAL_ERROR = 30
EXIT_TRANSLATE_INPUT_ERROR = 31
EXIT_SEARCH_CRITICAL_ERROR = 32
EXIT_SEARCH_INPUT_ERROR = 33
EXIT_SEARCH_UNSUPPORTED = 34


def successful_execution(exitcode):
    # Exit codes from 0 to 9 are those that represent a successful execution,
    # i.e. a completed translator run or a found plan.
    return exitcode in range(10)


def unsolvable(exitcode):
    # Exit codes in the range from 10 to 19 represent the cases where no
    # error occured but no plan could be found.
    return exitcode in range(10, 20)


def unrecoverable_exitcodes():
    # Exit codes in the range from 30 to 39 represent unrecoverable failures.
    return range(30, 40)


def generate_portfolio_exitcode(exitcodes):
    """A portfolio's exitcode is determined as follows:

    There is exactly one type of unexpected exit code -> use it.
    There are multiple types of unexpected exit codes -> EXIT_SEARCH_CRITICAL_ERROR.
    [..., EXIT_SUCCESS, ...] -> EXIT_SUCCESS
    [..., EXIT_SEARCH_UNSOLVABLE, ...] -> EXIT_SEARCH_UNSOLVABLE
    [..., EXIT_SEARCH_UNSOLVED_INCOMPLETE, ...] -> EXIT_SEARCH_UNSOLVED_INCOMPLETE
    [..., EXIT_SEARCH_OUT_OF_MEMORY, ..., EXIT_SEARCH_OUT_OF_TIME, ...] -> EXIT_SEARCH_OUT_OF_MEMORY_AND_TIME
    [..., EXIT_SEARCH_OUT_OF_TIME, ...] -> EXIT_SEARCH_OUT_OF_TIME
    [..., EXIT_SEARCH_OUT_OF_MEMORY, ...] -> EXIT_SEARCH_OUT_OF_MEMORY
    """
    print("Exit codes: %s" % exitcodes)
    exitcodes = set(exitcodes)
    if EXIT_SEARCH_SIGXCPU in exitcodes:
        # TODO: why do we do this only for portfolios?
        exitcodes.remove(EXIT_SEARCH_SIGXCPU)
        exitcodes.add(EXIT_SEARCH_OUT_OF_TIME)
    unrecoverable_codes = exitcodes & set(unrecoverable_exitcodes())

    # There are unrecoverable exit codes.
    if unrecoverable_codes:
        print("Error: Unexpected exit codes: %s" % list(unrecoverable_codes))
        if len(unrecoverable_codes) == 1:
            return (unrecoverable_codes.pop(), False)
        else:
            return (EXIT_SEARCH_CRITICAL_ERROR, False)

    # At least one plan was found.
    if EXIT_SUCCESS in exitcodes:
        if EXIT_SEARCH_OUT_OF_MEMORY in exitcodes and EXIT_SEARCH_OUT_OF_TIME in exitcodes:
            return (EXIT_SEARCH_PLAN_FOUND_AND_OUT_OF_MEMORY_AND_TIME, True)
        elif EXIT_SEARCH_OUT_OF_MEMORY in exitcodes:
            return (EXIT_SEARCH_PLAN_FOUND_AND_OUT_OF_MEMORY, True)
        elif EXIT_SEARCH_OUT_OF_TIME in exitcodes:
            return (EXIT_SEARCH_PLAN_FOUND_AND_OUT_OF_TIME, True)
        else:
            return (EXIT_SUCCESS, True)

    # A config proved unsolvability or did not find a plan.
    for code in [EXIT_SEARCH_UNSOLVABLE, EXIT_SEARCH_UNSOLVED_INCOMPLETE]:
        if code in exitcodes:
            return (code, False)

    # No plan was found due to hitting resource limits.
    if EXIT_SEARCH_OUT_OF_MEMORY in exitcodes and EXIT_SEARCH_OUT_OF_TIME in exitcodes:
        return (EXIT_SEARCH_OUT_OF_MEMORY_AND_TIME, False)
    elif EXIT_SEARCH_OUT_OF_MEMORY in exitcodes:
        return (EXIT_SEARCH_OUT_OF_MEMORY, False)
    elif EXIT_SEARCH_OUT_OF_TIME in exitcodes:
        return (EXIT_SEARCH_OUT_OF_TIME, False)

    assert False, "Error: Unhandled exit codes: %s" % exitcodes
