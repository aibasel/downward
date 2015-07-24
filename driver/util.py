# -*- coding: utf-8 -*-

import os

DRIVER_DIR = os.path.abspath(os.path.dirname(__file__))
SRC_DIR = os.path.dirname(DRIVER_DIR)


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
