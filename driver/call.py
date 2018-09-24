# -*- coding: utf-8 -*-

from __future__ import print_function

"""Make subprocess calls with time and memory limits."""

from . import limits
from . import util

import logging
import subprocess
import sys


def print_call_settings(nick, cmd, stdin, time_limit, memory_limit):
    if stdin is not None:
        stdin = util.shell_escape(stdin)
    logging.info("{} stdin: {}".format(nick, stdin))
    if time_limit is not None:
        time_limit = str(time_limit) + "s"
    logging.info("{} time limit: {}".format(nick, time_limit))
    if memory_limit is not None:
        memory_limit = int(limits.convert_to_mb(memory_limit))
        memory_limit = str(memory_limit) + " MB"
    logging.info("{} memory limit: {}".format(nick, memory_limit))

    escaped_cmd = [util.shell_escape(x) for x in cmd]
    if stdin is not None:
        escaped_cmd.extend(["<", util.shell_escape(stdin)])
    logging.info("{} command line string: {}".format(nick, " ".join(escaped_cmd)))


def _get_preexec_function(time_limit, memory_limit):
    def set_limits():
        limits.set_time_limit(time_limit)
        limits.set_memory_limit(memory_limit)

    if time_limit is None and memory_limit is None:
        return None
    else:
        return set_limits


def check_call(nick, cmd, stdin=None, time_limit=None, memory_limit=None):
    print_call_settings(nick, cmd, stdin, time_limit, memory_limit)

    kwargs = {"preexec_fn": _get_preexec_function(time_limit, memory_limit)}

    sys.stdout.flush()
    if stdin:
        with open(stdin) as stdin_file:
            return subprocess.check_call(cmd, stdin=stdin_file, **kwargs)
    else:
        return subprocess.check_call(cmd, **kwargs)


def get_error_output_and_returncode(nick, cmd, time_limit=None, memory_limit=None):
    print_call_settings(nick, cmd, None, time_limit, memory_limit)

    preexec_fn = _get_preexec_function(time_limit, memory_limit)

    sys.stdout.flush()
    p = subprocess.Popen(cmd, preexec_fn=preexec_fn, stderr=subprocess.PIPE)
    (stdout, stderr) = p.communicate()
    return stderr, p.returncode
