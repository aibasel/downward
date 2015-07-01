# -*- coding: utf-8 -*-

from __future__ import print_function

"""Make subprocess calls with time and memory limits."""

import math
try:
    import resource
except ImportError:
    resource = None
import subprocess
import sys


def _set_limit(kind, soft, hard=None):
    if hard is None:
        hard = soft
    try:
        resource.setrlimit(kind, (soft, hard))
    except (OSError, ValueError) as err:
        # This can happen if the limit has already been set externally.
        print("Limit for %s could not be set to %s (%s). Previous limit: %s" %
              (kind, (soft, hard), err, resource.getrlimit(kind)), file=sys.stderr)


def _set_time_limit(timeout):
    # TODO: Raise soft limit if timeout=None
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


def _set_memory_limit(memory):
    if memory is None:
        memory = resource.RLIM_INFINITY
    # Memory in bytes.
    _set_limit(resource.RLIMIT_AS, memory)


def check_call(cmd, stdin=None, timeout=None, memory=None):
    if (timeout is not None or memory is not None) and resource is None:
        raise NotImplementedError(
            "The 'resource' module is not available on your platform. "
            "Therefore, setting time or memory limits, and running "
            "portfolios is not possible.")
    sys.stdout.flush()

    def set_limits():
        _set_time_limit(timeout)
        _set_memory_limit(memory)

    kwargs = dict(preexec_fn=set_limits)

    if stdin:
        with open(stdin) as stdin_file:
            return subprocess.check_call(cmd, stdin=stdin_file, **kwargs)
    else:
        return subprocess.check_call(cmd, **kwargs)
