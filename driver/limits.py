import logging
try:
    import resource
except ImportError:
    resource = None
import sys

from . import returncodes
from . import util


"""
Notes on limits: On Windows, the resource module does not exist and hence we
cannot enforce any limits there. Furthermore, while the module exists on macOS,
memory limits are not enforced by that OS and hence we do not support imposing
memory limits there.
"""

CANNOT_LIMIT_MEMORY_MSG = "Setting memory limits is not supported on your platform."
CANNOT_LIMIT_TIME_MSG = "Setting time limits is not supported on your platform."


def can_set_time_limit():
    return resource is not None


def can_set_memory_limit():
    return resource is not None and sys.platform != "darwin"


def set_time_limit(time_limit):
    if time_limit is None:
        return
    if not can_set_time_limit():
        raise NotImplementedError(CANNOT_LIMIT_TIME_MSG)
    # Reaching the soft time limit leads to a (catchable) SIGXCPU signal,
    # which we catch to gracefully exit. Reaching the hard limit leads to
    # a SIGKILL, which is unpreventable. We set a hard limit one second
    # higher than the soft limit to make sure we abort also in cases where
    # the graceful shutdown doesn't work, or doesn't work reasonably
    # quickly.
    try:
        resource.setrlimit(resource.RLIMIT_CPU, (time_limit, time_limit + 1))
    except ValueError:
        # If the previous call failed, we try again without the extra second.
        # In particular, this is necessary if there already exists an external
        # hard limit equal to time_limit.
        resource.setrlimit(resource.RLIMIT_CPU, (time_limit, time_limit))


def set_memory_limit(memory):
    """*memory* must be given in bytes or None."""
    if memory is None:
        return
    if not can_set_memory_limit():
        raise NotImplementedError(CANNOT_LIMIT_MEMORY_MSG)
    resource.setrlimit(resource.RLIMIT_AS, (memory, memory))


def convert_to_mb(num_bytes):
    return num_bytes / (1024 * 1024)


def get_memory_limit(component_limit, overall_limit):
    """
    Return the minimum of the component and overall limits or None if neither is set.
    """
    limits = [limit for limit in [component_limit, overall_limit] if limit is not None]
    return min(limits) if limits else None


def get_time_limit(component_limit, overall_limit):
    """
    Return the minimum time limit imposed by the component and overall limits.
    """
    limit = component_limit
    if overall_limit is not None:
        try:
            elapsed_time = util.get_elapsed_time()
        except NotImplementedError:
            returncodes.exit_with_driver_unsupported_error(CANNOT_LIMIT_TIME_MSG)
        else:
            remaining_time = max(0, overall_limit - elapsed_time)
            if limit is None or remaining_time < limit:
                limit = remaining_time
    return limit


def print_limits(nick, time_limit, memory_limit):
    if time_limit is not None:
        time_limit = str(time_limit) + "s"
    logging.info("{} time limit: {}".format(nick, time_limit))
    if memory_limit is not None:
        memory_limit = int(convert_to_mb(memory_limit))
        memory_limit = str(memory_limit) + " MB"
    logging.info("{} memory limit: {}".format(nick, memory_limit))
