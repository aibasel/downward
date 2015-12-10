# -*- coding: utf-8 -*-

import os
import re


DRIVER_DIR = os.path.abspath(os.path.dirname(__file__))
REPO_ROOT_DIR = os.path.dirname(DRIVER_DIR)
BUILDS_DIR = os.path.join(REPO_ROOT_DIR, "builds")


def get_elapsed_time():
    """
    Return the CPU time taken by the python process and its child
    processes.
    """
    if os.name == "nt":
        # The child time components of os.times() are 0 on Windows. If
        # we ever end up using this method on Windows, we need to be
        # aware of this, so it's prudent to complain loudly.
        raise NotImplementedError("cannot use get_elapsed_time() on Windows")
    return sum(os.times()[:4])


def find_domain_filename(task_filename):
    """
    Find domain filename for the given task using automatic naming rules.
    """
    dirname, basename = os.path.split(task_filename)

    domain_basenames = [
        "domain.pddl",
        basename[:3] + "-domain.pddl",
        "domain_" + basename,
    ]

    for domain_basename in domain_basenames:
        domain_filename = os.path.join(dirname, domain_basename)
        if os.path.exists(domain_filename):
            return domain_filename
            
    raise SystemExit(
        "Error: Could not find domain file using automatic naming rules.")

# Shell-escaping code taken from Python's shlex.quote (missing in Python < 3.3).
_find_unsafe = re.compile(r'[^\w@%+=:,./-]').search

def shell_escape(s):
    """Return a shell-escaped version of the string *s*."""
    if not s:
        return "''"
    if _find_unsafe(s) is None:
        return s

    # Use single quotes, and put single quotes into double quotes.
    # The string $'b is then quoted as '$'"'"'b'.
    return "'" + s.replace("'", "'\"'\"'") + "'"
