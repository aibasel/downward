# -*- coding: utf-8 -*-

from __future__ import print_function

import signal

## Exit codes denoting successful execution allowing to continue with other
## components: translator completed and/or a plan was found

EXIT_SUCCESS = 0 # translator completed, or search found a plan, or validate validated a plan.
EXIT_SEARCH_PLAN_FOUND_AND_OUT_OF_MEMORY = 1 # only for anytime search configurations
EXIT_SEARCH_PLAN_FOUND_AND_OUT_OF_TIME = 2 # only for anytime search configurations
EXIT_SEARCH_PLAN_FOUND_AND_OUT_OF_MEMORY_AND_TIME = 3 # only for anytime search configurations

## Exit codes denoting no plan can exist and hence no execution of other
## components is necessary.

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

EXIT_TRANSLATE_CRITICAL = 30
EXIT_SEARCH_CRITICAL_ERROR = 31
EXIT_SEARCH_INPUT_ERROR = 32
EXIT_SEARCH_UNSUPPORTED = 33


EXPECTED_SEARCH_EXITCODES = set([
    EXIT_SUCCESS,
    EXIT_SEARCH_PLAN_FOUND_AND_OUT_OF_MEMORY,
    EXIT_SEARCH_PLAN_FOUND_AND_OUT_OF_TIME,
    EXIT_SEARCH_PLAN_FOUND_AND_OUT_OF_MEMORY_AND_TIME,
    EXIT_SEARCH_UNSOLVABLE,
    EXIT_SEARCH_UNSOLVED_INCOMPLETE,
    EXIT_SEARCH_OUT_OF_MEMORY,
    EXIT_SEARCH_OUT_OF_TIME,
    EXIT_SEARCH_SIGXCPU,
    EXIT_SEARCH_OUT_OF_MEMORY_AND_TIME]
)


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
    unexpected_codes = exitcodes - EXPECTED_SEARCH_EXITCODES

    # Stop execution in the presence of unexpected exit codes.
    if unexpected_codes:
        print("Error: Unexpected exit codes: %s" % list(unexpected_codes))
        if len(unexpected_codes) == 1:
            return (unexpected_codes.pop(), False)
        else:
            return (EXIT_SEARCH_CRITICAL_ERROR, False)

    # If at least one plan was found, continue execution.
    for code in [EXIT_SUCCESS, EXIT_SEARCH_PLAN_FOUND_AND_OUT_OF_MEMORY, EXIT_SEARCH_PLAN_FOUND_AND_OUT_OF_TIME, EXIT_SEARCH_PLAN_FOUND_AND_OUT_OF_MEMORY_AND_TIME]:
        if code in exitcodes:
            return (code, True)

    # If a config proved unsolvability, do not continue execution.
    for code in [EXIT_SEARCH_UNSOLVABLE, EXIT_SEARCH_UNSOLVED_INCOMPLETE]:
        if code in exitcodes:
            return (code, False)

    # If no plan was found due to hitting resource limits, do not continue execution.
    for code in [EXIT_SEARCH_OUT_OF_MEMORY, EXIT_SEARCH_OUT_OF_TIME]:
        if exitcodes == set([code]):
            return (code, False)
    if exitcodes == set([EXIT_SEARCH_OUT_OF_MEMORY, EXIT_SEARCH_OUT_OF_TIME]):
        return (EXIT_SEARCH_OUT_OF_MEMORY_AND_TIME, False)

    # Do not continue execution if there are still unhandled exit codes.
    print("Error: Unhandled exit codes: %s" % exitcodes)
    return (EXIT_SEARCH_CRITICAL_ERROR, False)
