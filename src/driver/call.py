# -*- coding: utf-8 -*-

from __future__ import print_function

"""Make subprocess calls with time and memory limits."""

from . import limits

import subprocess
import sys


def check_call(cmd, stdin=None, time_limit=None, memory_limit=None):
    def set_limits():
        limits.set_time_limit(time_limit)
        limits.set_memory_limit(memory_limit)

    kwargs = {}
    if time_limit is not None or memory_limit is not None:
        if limits.can_set_limits():
            kwargs["preexec_fn"] = set_limits
        else:
            sys.exit(limits.RESOURCE_MODULE_MISSING_MSG)

    sys.stdout.flush()
    if stdin:
        with open(stdin) as stdin_file:
            return subprocess.check_call(cmd, stdin=stdin_file, **kwargs)
    else:
        return subprocess.check_call(cmd, **kwargs)
