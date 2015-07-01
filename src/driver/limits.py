# -*- coding: utf-8 -*-

from __future__ import print_function

import os
import resource


def get_min(values):
    """Filter None values and return minimum."""
    values = [val for val in values if val is not None]
    if values:
        return min(values)
    return None


def get_elapsed_time():
    """
    Return the CPU time taken by the python process and its child
    processes.

    Note: According to the os.times documentation, Windows sets the
    child time components to 0, so if we ever support running
    portfolios on Windows, time slices will be allocated slightly
    wrongly there.
    """
    return sum(os.times()[:4])


def get_external_timeout():
    # Time limits are either positive values in seconds or -1 (RLIM_INFINITY).
    soft, hard = resource.getrlimit(resource.RLIMIT_CPU)
    if soft != resource.RLIM_INFINITY:
        return soft
    elif hard != resource.RLIM_INFINITY:
        return hard
    else:
        return None

def get_external_hard_memory_limit():
    # Memory limits are either positive values in bytes or -1 (RLIM_INFINITY).
    _, hard_mem_limit = resource.getrlimit(resource.RLIMIT_AS)
    if hard_mem_limit == resource.RLIM_INFINITY:
        return None
    return hard_mem_limit


def _get_timeout_in_seconds(timeout_string):
    # TODO: Handle suffixes.
    return int(timeout_string)

def _get_memory_limit_in_bytes(parser, limit_string):
    # TODO: Handle suffixes.
    return int(limit_string)


def set_integer_timeout(parser, args, component):
    param = component + "_timeout"
    timeout = getattr(args, param)
    if timeout is not None:
        setattr(args, param, _get_timeout_in_seconds(timeout))

def set_integer_memory_limit(parser, args, component):
    param = component + "_memory"
    limit = getattr(args, param)
    if limit is not None:
        setattr(args, param, _get_memory_limit_in_bytes(parser, limit))


def get_memory_limit(component_limit, overall_limit):
    external_limit = get_external_hard_memory_limit()
    return get_min([component_limit, overall_limit, external_limit])


def get_timeout(component_timeout, overall_timeout):
    if overall_timeout is not None:
        overall_timeout = max(0, overall_timeout - get_elapsed_time())
    return get_min([component_timeout, overall_timeout])
