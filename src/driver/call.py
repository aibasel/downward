# -*- coding: utf-8 -*-

from __future__ import print_function

"""Make subprocess calls with time and memory limits."""

import limits

import subprocess
import sys


def check_call(cmd, stdin=None, timeout=None, memory=None):
    def set_limits():
        limits.set_time_limit(timeout)
        limits.set_memory_limit(memory)

    kwargs = {}
    if limits.can_set_limits():
        kwargs["preexec_fn"] = set_limits
    elif timeout is not None or memory is not None:
        raise NotImplementedError(
            "The 'resource' module is not available on your platform. "
            "Therefore, setting time or memory limits, and running "
            "portfolios is not possible.")

    sys.stdout.flush()
    if stdin:
        with open(stdin) as stdin_file:
            return subprocess.check_call(cmd, stdin=stdin_file, **kwargs)
    else:
        return subprocess.check_call(cmd, **kwargs)
