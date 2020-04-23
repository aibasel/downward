from collections import defaultdict
import os
import subprocess
import sys

import pytest

DIR = os.path.dirname(os.path.abspath(__file__))
REPO_BASE = os.path.dirname(os.path.dirname(DIR))

sys.path.insert(0, REPO_BASE)
from driver import returncodes

BENCHMARKS_DIR = os.path.join(REPO_BASE, "misc", "tests", "benchmarks")
DRIVER = os.path.join(REPO_BASE, "fast-downward.py")

TRANSLATE_TASKS = {
    "small": "gripper/prob01.pddl",
    "large": "satellite/p25-HC-pfile5.pddl",
}

TRANSLATE_TESTS = [
    ("small", [], [], defaultdict(lambda: returncodes.SUCCESS)),
    # We cannot set time limits on Windows and thus expect DRIVER_UNSUPPORTED
    # as exit code in this case.
    ("large", ["--translate-time-limit", "1s"], [], defaultdict(
        lambda: returncodes.TRANSLATE_OUT_OF_TIME,
        win32=returncodes.DRIVER_UNSUPPORTED)),
    # We cannot set/enforce memory limits on Windows/macOS and thus expect
    # DRIVER_UNSUPPORTED as exit code in those cases.
    ("large", ["--translate-memory-limit", "75M"], [], defaultdict(
        lambda: returncodes.TRANSLATE_OUT_OF_MEMORY,
        darwin=returncodes.DRIVER_UNSUPPORTED,
        win32=returncodes.DRIVER_UNSUPPORTED)),
]

SEARCH_TASKS = {
    "strips": "miconic/s1-0.pddl",
    "axioms": "philosophers/p01-phil2.pddl",
    "cond-eff": "miconic-simpleadl/s1-0.pddl",
    "large": "satellite/p25-HC-pfile5.pddl",
}

MERGE_AND_SHRINK = ('astar(merge_and_shrink('
    'merge_strategy=merge_stateless(merge_selector='
        'score_based_filtering(scoring_functions=[goal_relevance,'
        'dfp,total_order(atomic_ts_order=reverse_level,'
        'product_ts_order=new_to_old,atomic_before_product=false)])),'
    'shrink_strategy=shrink_bisimulation(greedy=false),'
    'label_reduction=exact('
        'before_shrinking=true,'
        'before_merging=false),'
    'max_states=50000,threshold_before_merge=1,verbosity=silent))')

SEARCH_TESTS = [
    ("strips", [], "astar(add())", defaultdict(lambda: returncodes.SUCCESS)),
    ("strips", [], "astar(hm())", defaultdict(lambda: returncodes.SUCCESS)),
    ("strips", [], "ehc(hm())", defaultdict(lambda: returncodes.SUCCESS)),
    ("strips", [], "astar(ipdb())", defaultdict(lambda: returncodes.SUCCESS)),
    ("strips", [], "astar(lmcut())", defaultdict(lambda: returncodes.SUCCESS)),
    ("strips", [], "astar(lmcount(lm_rhw(), admissible=false))",
        defaultdict(lambda: returncodes.SUCCESS)),
    ("strips", [], "astar(lmcount(lm_rhw(), admissible=true))",
        defaultdict(lambda: returncodes.SUCCESS)),
    ("strips", [], "astar(lmcount(lm_hm(), admissible=false))",
        defaultdict(lambda: returncodes.SUCCESS)),
    ("strips", [], "astar(lmcount(lm_hm(), admissible=true))",
        defaultdict(lambda: returncodes.SUCCESS)),
    ("strips", [], MERGE_AND_SHRINK, defaultdict(lambda: returncodes.SUCCESS)),
    ("axioms", [], "astar(add())", defaultdict(lambda: returncodes.SUCCESS)),
    ("axioms", [], "astar(hm())",
        defaultdict(lambda: returncodes.SEARCH_UNSOLVED_INCOMPLETE)),
    ("axioms", [], "ehc(hm())",
        defaultdict(lambda: returncodes.SEARCH_UNSOLVED_INCOMPLETE)),
    ("axioms", [], "astar(ipdb())",
        defaultdict(lambda: returncodes.SEARCH_UNSUPPORTED)),
    ("axioms", [], "astar(lmcut())",
        defaultdict(lambda: returncodes.SEARCH_UNSUPPORTED)),
    ("axioms", [], "astar(lmcount(lm_rhw(), admissible=false))",
        defaultdict(lambda: returncodes.SUCCESS)),
    ("axioms", [], "astar(lmcount(lm_rhw(), admissible=true))",
        defaultdict(lambda: returncodes.SEARCH_UNSUPPORTED)),
    ("axioms", [], "astar(lmcount(lm_zg(), admissible=false))",
        defaultdict(lambda: returncodes.SUCCESS)),
    ("axioms", [], "astar(lmcount(lm_zg(), admissible=true))",
        defaultdict(lambda: returncodes.SEARCH_UNSUPPORTED)),
    # h^m landmark factory explicitly forbids axioms.
    ("axioms", [], "astar(lmcount(lm_hm(), admissible=false))",
        defaultdict(lambda: returncodes.SEARCH_UNSUPPORTED)),
    ("axioms", [], "astar(lmcount(lm_hm(), admissible=true))",
        defaultdict(lambda: returncodes.SEARCH_UNSUPPORTED)),
    ("axioms", [], "astar(lmcount(lm_exhaust(), admissible=false))",
        defaultdict(lambda: returncodes.SUCCESS)),
    ("axioms", [], "astar(lmcount(lm_exhaust(), admissible=true))",
        defaultdict(lambda: returncodes.SEARCH_UNSUPPORTED)),
    ("axioms", [], MERGE_AND_SHRINK,
        defaultdict(lambda: returncodes.SEARCH_UNSUPPORTED)),
    ("cond-eff", [], "astar(add())",
        defaultdict(lambda: returncodes.SUCCESS)),
    ("cond-eff", [], "astar(hm())",
        defaultdict(lambda: returncodes.SUCCESS)),
    ("cond-eff", [], "astar(ipdb())",
        defaultdict(lambda: returncodes.SEARCH_UNSUPPORTED)),
    ("cond-eff", [], "astar(lmcut())",
        defaultdict(lambda: returncodes.SEARCH_UNSUPPORTED)),
    ("cond-eff", [], "astar(lmcount(lm_rhw(), admissible=false))",
        defaultdict(lambda: returncodes.SUCCESS)),
    ("cond-eff", [], "astar(lmcount(lm_rhw(), admissible=true))",
        defaultdict(lambda: returncodes.SUCCESS)),
    ("cond-eff", [], "astar(lmcount(lm_zg(), admissible=false))",
        defaultdict(lambda: returncodes.SUCCESS)),
    ("cond-eff", [], "astar(lmcount(lm_zg(), admissible=true))",
        defaultdict(lambda: returncodes.SUCCESS)),
    ("cond-eff", [], "astar(lmcount(lm_hm(), admissible=false))",
        defaultdict(lambda: returncodes.SUCCESS)),
    ("cond-eff", [], "astar(lmcount(lm_hm(), admissible=true))",
        defaultdict(lambda: returncodes.SEARCH_UNSUPPORTED)),
    ("cond-eff", [], "astar(lmcount(lm_exhaust(), admissible=false))",
        defaultdict(lambda: returncodes.SUCCESS)),
    ("cond-eff", [], "astar(lmcount(lm_exhaust(), admissible=true))",
        defaultdict(lambda: returncodes.SEARCH_UNSUPPORTED)),
    ("cond-eff", [], MERGE_AND_SHRINK,
        defaultdict(lambda: returncodes.SUCCESS)),
    # We cannot set/enforce memory limits on Windows/macOS and thus expect
    # DRIVER_UNSUPPORTED as exit code in those cases.
    ("large", ["--search-memory-limit", "100M"], MERGE_AND_SHRINK,
        defaultdict(lambda: returncodes.SEARCH_OUT_OF_MEMORY,
                    darwin=returncodes.DRIVER_UNSUPPORTED,
                    win32=returncodes.DRIVER_UNSUPPORTED)),
    # We cannot set time limits on Windows and thus expect DRIVER_UNSUPPORTED
    # as exit code in this case.
    ("large", ["--search-time-limit", "1s"], MERGE_AND_SHRINK,
        defaultdict(lambda: returncodes.SEARCH_OUT_OF_TIME,
                    win32=returncodes.DRIVER_UNSUPPORTED)),
]


def translate(pddl_file, sas_file):
    subprocess.check_call([
        sys.executable, DRIVER, "--sas-file", sas_file, "--translate", pddl_file])


def cleanup():
    subprocess.check_call([sys.executable, DRIVER, "--cleanup"])


def get_sas_file_name(task_type):
    return "{}.sas".format(task_type)


def setup_module(_module):
    for task_type, relpath in SEARCH_TASKS.items():
        pddl_file = os.path.join(BENCHMARKS_DIR, relpath)
        sas_file = get_sas_file_name(task_type)
        translate(pddl_file, sas_file)


@pytest.mark.parametrize(
    "task_type, driver_options, translate_options, expected", TRANSLATE_TESTS)
def test_translator_exit_codes(task_type, driver_options, translate_options, expected):
    relpath = TRANSLATE_TASKS[task_type]
    problem = os.path.join(BENCHMARKS_DIR, relpath)
    cmd = ([sys.executable, DRIVER] + driver_options +
        ["--translate"] + translate_options + [problem])
    print("\nRun {cmd}:".format(**locals()))
    sys.stdout.flush()
    exitcode = subprocess.call(cmd)
    assert exitcode == expected[sys.platform]
    cleanup()


@pytest.mark.parametrize(
    "task_type, driver_options, search_options, expected", SEARCH_TESTS)
def test_search_exit_codes(task_type, driver_options, search_options, expected):
    sas_file = get_sas_file_name(task_type)
    cmd = ([sys.executable, DRIVER] + driver_options +
        [sas_file, "--search", search_options])
    print("\nRun {cmd}:".format(**locals()))
    sys.stdout.flush()
    exitcode = subprocess.call(cmd)
    assert exitcode == expected[sys.platform]
    cleanup()


def teardown_module(_module):
    for task_type in SEARCH_TASKS:
        os.remove(get_sas_file_name(task_type))
