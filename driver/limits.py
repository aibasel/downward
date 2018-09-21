# -*- coding: utf-8 -*-

from __future__ import division, print_function

from . import returncodes
from . import util

import math
try:
    import resource
except ImportError:
    resource = None
import sys


"""
Notes on limits: On Windows, the resource module does not exist and hence we
cannot enforce any limits there. Furthermore, while the module exists on macOS,
memory limits are not enforced by that OS and hence we do not support imposing
memory limits there.
"""

CANNOT_LIMIT_MEMORY_MSG = "Setting memory limits is not supported on your platform."
CANNOT_LIMIT_TIME_MSG = "Setting time limits is not supported on your platform."


def can_set_time_limit():
    return resource is not None


def can_set_memory_limit():
    return resource is not None and sys.platform != "darwin"


def _set_limit(kind, soft, hard=None):
    if hard is None:
        hard = soft
    try:
        resource.setrlimit(kind, (soft, hard))
    except (OSError, ValueError) as err:
        returncodes.exit_with_driver_critical_error(
            "Limit for {} could not be set to ({},{}) ({}). "
            "Previous limit: {}".format(
                kind, soft, hard, err, resource.getrlimit(kind)))


def _get_soft_and_hard_time_limits(internal_limit, external_hard_limit):
    soft_limit = min(int(math.ceil(internal_limit)), external_hard_limit)
    hard_limit = min(soft_limit + 1, external_hard_limit)
    print("time limit %.2f -> (%d, %d)" %
        (internal_limit, soft_limit, hard_limit))
    sys.stdout.flush()
    assert soft_limit <= hard_limit
    return soft_limit, hard_limit


def set_time_limit(time_limit):
    if time_limit is None:
        return
    if not can_set_time_limit():
        returncodes.exit_with_driver_unsupported_error(CANNOT_LIMIT_TIME_MSG)
    # Don't try to raise the hard limit.
    _, external_hard_limit = resource.getrlimit(resource.RLIMIT_CPU)
    if external_hard_limit == resource.RLIM_INFINITY:
        external_hard_limit = float("inf")
    assert time_limit <= external_hard_limit, (time_limit, external_hard_limit)
    # Soft limit reached --> SIGXCPU.
    # Hard limit reached --> SIGKILL.
    soft_limit, hard_limit = _get_soft_and_hard_time_limits(
        time_limit, external_hard_limit)
    _set_limit(resource.RLIMIT_CPU, soft_limit, hard_limit)


def set_memory_limit(memory):
    """*memory* must be given in bytes or None."""
    if memory is None:
        return
    if not can_set_memory_limit():
        returncodes.exit_with_driver_unsupported_error(CANNOT_LIMIT_MEMORY_MSG)
    _set_limit(resource.RLIMIT_AS, memory)


def convert_to_mb(num_bytes):
    return num_bytes / (1024 * 1024)


def _get_external_limit(kind):
    if resource is None:
        return None
    # Limits are either positive values or -1 (RLIM_INFINITY).
    soft, hard = resource.getrlimit(kind)
    if soft != resource.RLIM_INFINITY:
        return soft
    elif hard != resource.RLIM_INFINITY:
        return hard
    else:
        return None

def _get_external_time_limit():
    """Return external soft CPU limit in seconds or None if not set."""
    return _get_external_limit(resource.RLIMIT_CPU)

def _get_external_memory_limit():
    """Return external soft memory limit in bytes or None if not set."""
    return _get_external_limit(resource.RLIMIT_AS)


def get_memory_limit(component_limit, overall_limit):
    """
    Return the lowest of the following memory limits:
    component, overall, external soft, external hard.
    """
    if component_limit is None and overall_limit is None:
        return None
    elif can_set_memory_limit():
        limits = [component_limit, overall_limit, _get_external_memory_limit()]
        limits = [limit for limit in limits if limit is not None]
        return min(limits) if limits else None
    else:
        returncodes.exit_with_driver_unsupported_error(CANNOT_LIMIT_MEMORY_MSG)

def get_time_limit(component_limit, overall_limit):
    """
    Return the minimum time limit imposed by any internal and external limit.
    """
    if component_limit is None and overall_limit is None:
        return None
    elif can_set_time_limit():
        elapsed_time = util.get_elapsed_time()
        external_limit = _get_external_time_limit()
        limits = []
        if component_limit is not None:
            limits.append(component_limit)
        if overall_limit is not None:
            limits.append(max(0, overall_limit - elapsed_time))
        if external_limit is not None:
            limits.append(max(0, external_limit - elapsed_time))
        return min(limits) if limits else None
    else:
        returncodes.exit_with_driver_unsupported_error(CANNOT_LIMIT_TIME_MSG)
