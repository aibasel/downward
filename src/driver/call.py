# -*- coding: utf-8 -*-

from __future__ import print_function

"""Make subprocess calls with time and memory limits."""

import math
import resource
import subprocess
import sys


def set_limit(kind, soft, hard):
    try:
        resource.setrlimit(kind, (soft, hard))
    except (OSError, ValueError) as err:
        # This can happen if the limit has already been set externally.
        print("Limit for %s could not be set to %s (%s). Previous limit: %s" %
              (kind, (soft, hard), err, resource.getrlimit(kind)), file=sys.stderr)


def set_limits(timeout=None, memory=None):
    if timeout is not None:
        # Don't try to raise the hard limit.
        _, external_hard_limit = resource.getrlimit(resource.RLIMIT_CPU)
        if external_hard_limit == resource.RLIM_INFINITY:
            external_hard_limit = float("inf")
        # Soft limit reached --> SIGXCPU.
        # Hard limit reached --> SIGKILL.
        soft_limit = int(math.ceil(timeout))
        hard_limit = min(soft_limit + 1, external_hard_limit)
        print("timeout: %.2f -> (%d, %d)" % (timeout, soft_limit, hard_limit))
        sys.stdout.flush()
        set_limit(resource.RLIMIT_CPU, soft_limit, hard_limit)
    if memory is None:
        set_limit(resource.RLIMIT_AS, -1, -1)
    else:
        # Memory in bytes.
        set_limit(resource.RLIMIT_AS, memory, memory)


def check_call(cmd, stdin=None, timeout=None, memory=None):
    sys.stdout.flush()

    def set_limits_closure():
        set_limits(timeout=timeout, memory=memory)

    kwargs = dict(preexec_fn=set_limits_closure)

    if stdin:
        with open(stdin) as stdin_file:
            return subprocess.check_call(cmd, stdin=stdin_file, **kwargs)
    else:
        return subprocess.check_call(cmd, **kwargs)
