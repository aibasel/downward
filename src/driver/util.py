# -*- coding: utf-8 -*-

import os


def get_min(values):
    """
    Filter None values and return minimum. Return None if filtered list
    is empty.
    """
    values = [val for val in values if val is not None]
    if values:
        return min(values)
    return None


def get_elapsed_time():
    """
    Return the CPU time taken by the python process and its child
    processes.

    Note: According to the os.times documentation, Windows sets the
    child time components to 0, so if we ever support running
    portfolios on Windows, time slices will be allocated slightly
    wrongly there.
    """
    return sum(os.times()[:4])
