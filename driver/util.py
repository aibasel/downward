import os
from pathlib import Path

from . import returncodes


DRIVER_DIR = Path(__file__).parent.resolve()
REPO_ROOT_DIR = DRIVER_DIR.parent
BUILDS_DIR = REPO_ROOT_DIR / "builds"


def get_elapsed_time():
    """
    Return the CPU time taken by the python process and its child
    processes.
    """
    if os.name == "nt":
        # The child time components of os.times() are 0 on Windows.
        raise NotImplementedError("cannot use get_elapsed_time() on Windows")
    return sum(os.times()[:4])


def find_domain_path(task_path: Path):
    """
    Find domain path for the given task using automatic naming rules.
    """
    domain_basenames = [
        "domain.pddl",
        task_path.stem + "-domain" + task_path.suffix,
        task_path.name[:3] + "-domain.pddl", # for airport
        "domain_" + task_path.name,
        "domain-" + task_path.name,
    ]

    for domain_basename in domain_basenames:
        domain_path = task_path.parent / domain_basename
        if domain_path.exists():
            return domain_path

    returncodes.exit_with_driver_input_error(
        "Error: Could not find domain file using automatic naming rules.")
