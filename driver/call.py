"""Make subprocess calls with time and memory limits."""

from . import limits
from . import returncodes

import logging
import os
import shlex
import subprocess
import sys


def _replace_paths_with_strings(cmd):
    return [str(x) for x in cmd]


def print_call_settings(nick, cmd, stdin, time_limit, memory_limit):
    cmd = _replace_paths_with_strings(cmd)
    if stdin is not None:
        stdin = shlex.quote(str(stdin))
    logging.info("{} stdin: {}".format(nick, stdin))
    limits.print_limits(nick, time_limit, memory_limit)

    escaped_cmd = [shlex.quote(x) for x in cmd]
    if stdin is not None:
        escaped_cmd.extend(["<", shlex.quote(stdin)])
    logging.info("{} command line string: {}".format(nick, " ".join(escaped_cmd)))


def _get_preexec_function(time_limit, memory_limit):
    def set_limits():
        def _try_or_exit(function, description):
            def fail(exception, exitcode):
                returncodes.print_stderr("{} failed: {}".format(description, exception))
                os._exit(exitcode)
            try:
                function()
            except NotImplementedError as err:
                fail(err, returncodes.DRIVER_UNSUPPORTED)
            except OSError as err:
                fail(err, returncodes.DRIVER_CRITICAL_ERROR)
            except ValueError as err:
                fail(err, returncodes.DRIVER_INPUT_ERROR)

        _try_or_exit(lambda: limits.set_time_limit(time_limit), "Setting time limit")
        _try_or_exit(lambda: limits.set_memory_limit(memory_limit), "Setting memory limit")

    if time_limit is None and memory_limit is None:
        return None
    else:
        return set_limits


def check_call(nick, cmd, stdin=None, time_limit=None, memory_limit=None):
    cmd = _replace_paths_with_strings(cmd)
    print_call_settings(nick, cmd, stdin, time_limit, memory_limit)

    kwargs = {"preexec_fn": _get_preexec_function(time_limit, memory_limit)}

    sys.stdout.flush()
    if stdin:
        with open(stdin) as stdin_file:
            return subprocess.check_call(cmd, stdin=stdin_file, **kwargs)
    else:
        return subprocess.check_call(cmd, **kwargs)


def get_error_output_and_returncode(nick, cmd, time_limit=None, memory_limit=None):
    cmd = _replace_paths_with_strings(cmd)
    print_call_settings(nick, cmd, None, time_limit, memory_limit)

    preexec_fn = _get_preexec_function(time_limit, memory_limit)

    sys.stdout.flush()
    p = subprocess.Popen(cmd, preexec_fn=preexec_fn, stderr=subprocess.PIPE)
    (stdout, stderr) = p.communicate()
    return stderr, p.returncode
