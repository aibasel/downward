# -*- coding: utf-8 -*-

from __future__ import division, print_function

import util

import math
import re
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
    assert can_set_limits()
    # Don't try to raise the hard limit.
    _, external_hard_limit = resource.getrlimit(resource.RLIMIT_CPU)
    if external_hard_limit == resource.RLIM_INFINITY:
        external_hard_limit = float("inf")
    assert timeout <= external_hard_limit, (timeout, external_hard_limit)
    # Soft limit reached --> SIGXCPU.
    # Hard limit reached --> SIGKILL.
    soft_limit = int(math.ceil(timeout))
    hard_limit = min(soft_limit + 1, external_hard_limit)
    print("timeout %.2f -> (%d, %d)" % (timeout, soft_limit, hard_limit))
    sys.stdout.flush()
    _set_limit(resource.RLIMIT_CPU, soft_limit, hard_limit)


def set_memory_limit(memory):
    """*memory* must be given in bytes or None."""
    if memory is None:
        return
    assert can_set_limits()
    _set_limit(resource.RLIMIT_AS, memory)


def convert_to_mb(num_bytes):
    return num_bytes / (1024 * 1024)


def _get_external_limit(kind):
    if not can_set_limits():
        return None
    # Limits are either positive values or -1 (RLIM_INFINITY).
    soft, hard = resource.getrlimit(kind)
    if soft != resource.RLIM_INFINITY:
        return soft
    elif hard != resource.RLIM_INFINITY:
        return hard
    else:
        return None

def _get_external_timeout():
    """Return external soft CPU limit in seconds or None if not set."""
    if not can_set_limits():
        return None
    return _get_external_limit(resource.RLIMIT_CPU)

def _get_external_memory_limit():
    """Return external soft memory limit in bytes or None if not set."""
    if not can_set_limits():
        return None
    return _get_external_limit(resource.RLIMIT_AS)


def _get_timeout_in_seconds(limit, parser):
    match = re.match(r"^(\d+)(s|m|h)?$", limit, flags=re.I)
    if not match:
        parser.error("malformed timeout parameter: {}".format(limit))
    time = int(match.group(1))
    suffix = match.group(2)
    if suffix is not None:
        suffix = suffix.lower()
    if suffix == "m":
        time *= 60
    elif suffix == "h":
        time *= 3600
    return time

def _get_memory_limit_in_bytes(limit, parser):
    match = re.match(r"^(\d+)(k|m|g)?$", limit, flags=re.I)
    if not match:
        parser.error("malformed memory parameter: {}".format(limit))
    memory = int(match.group(1))
    suffix = match.group(2)
    if suffix is not None:
        suffix = suffix.lower()
    if suffix == "k":
        memory *= 1024
    elif suffix is None or suffix == "m":
        memory *= 1024 * 1024
    elif suffix == "g":
        memory *= 1024 * 1024 * 1024
    return memory


def set_timeout_in_seconds(parser, args, component):
    param = component + "_timeout"
    timeout = getattr(args, param)
    if timeout is not None:
        setattr(args, param, _get_timeout_in_seconds(timeout, parser))

def set_memory_limit_in_bytes(parser, args, component):
    param = component + "_memory"
    limit = getattr(args, param)
    if limit is not None:
        setattr(args, param, _get_memory_limit_in_bytes(limit, parser))


def get_memory_limit(component_limit, overall_limit):
    """
    Return the lowest of the following memory limits:
    component, overall, external soft, external hard.
    """
    limits = [component_limit, overall_limit, _get_external_memory_limit()]
    limits = [limit for limit in limits if limit is not None]
    return min(limits) if limits else None

def get_timeout(component_timeout, overall_timeout):
    """
    Return the minimum timeout imposed by any internal and external limit.
    """
    elapsed_time = util.get_elapsed_time()
    external_timeout = _get_external_timeout()
    timeouts = []
    if component_timeout is not None:
        timeouts.append(component_timeout)
    if overall_timeout is not None:
        timeouts.append(max(0, overall_timeout - elapsed_time))
    if external_timeout is not None:
        timeouts.append(max(0, external_timeout - elapsed_time))
    return min(timeouts) if timeouts else None
