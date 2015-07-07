# -*- coding: utf-8 -*-

from __future__ import print_function

import util

import math
try:
    import resource
except ImportError:
    resource = None
import sys


def can_set_limits():
    return resource is not None


def _set_limit(kind, soft, hard=None):
    if hard is None:
        hard = soft
    try:
        resource.setrlimit(kind, (soft, hard))
    except (OSError, ValueError) as err:
        print(
            "Limit for {} could not be set to ({},{}) ({}). "
            "Previous limit: {}".format(
                kind, soft, hard, err, resource.getrlimit(kind)),
            file=sys.stderr)


def set_time_limit(timeout):
    if timeout is None:
        return
    # Don't try to raise the hard limit.
    _, external_hard_limit = resource.getrlimit(resource.RLIMIT_CPU)
    if external_hard_limit == resource.RLIM_INFINITY:
        external_hard_limit = float("inf")
    # Soft limit reached --> SIGXCPU.
    # Hard limit reached --> SIGKILL.
    soft_limit = int(math.ceil(timeout))
    hard_limit = min(soft_limit + 1, external_hard_limit)
    print("timeout %.2f -> (%d, %d)" % (timeout, soft_limit, hard_limit))
    sys.stdout.flush()
    _set_limit(resource.RLIMIT_CPU, soft_limit, hard_limit)


def set_memory_limit(memory):
    if memory is None:
        memory = resource.RLIM_INFINITY
    # Memory in bytes.
    _set_limit(resource.RLIMIT_AS, memory)


# TODO: Remove?
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


def set_timeout_in_seconds(parser, args, component):
    param = component + "_timeout"
    timeout = getattr(args, param)
    if timeout is not None:
        setattr(args, param, _get_timeout_in_seconds(timeout))

def set_memory_limit_in_bytes(parser, args, component):
    param = component + "_memory"
    limit = getattr(args, param)
    if limit is not None:
        setattr(args, param, _get_memory_limit_in_bytes(parser, limit))


def get_memory_limit(component_limit, overall_limit):
    external_limit = get_external_hard_memory_limit()
    return util.get_min([component_limit, overall_limit, external_limit])


def get_timeout(component_timeout, overall_timeout):
    if overall_timeout is not None:
        overall_timeout = max(0, overall_timeout - util.get_elapsed_time())
    return util.get_min([component_timeout, overall_timeout])
