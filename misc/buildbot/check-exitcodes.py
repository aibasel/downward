#! /usr/bin/env python

import os
import subprocess
import sys

DIR = os.path.dirname(os.path.abspath(__file__))
REPO_BASE = os.path.dirname(os.path.dirname(DIR))
BENCHMARKS_DIR = os.path.join(REPO_BASE, "benchmarks")

TASKS = {
    "strips": "gripper/prob01.pddl",
    "axioms": "philosophers/p01-phil2.pddl",
    "cond-eff": "miconic-simpleadl/s1-0.pddl",
}

EXIT_PLAN_FOUND = 0
EXIT_CRITICAL_ERROR = 1
EXIT_INPUT_ERROR = 2
EXIT_UNSUPPORTED = 3
EXIT_UNSOLVABLE = 4
EXIT_UNSOLVED_INCOMPLETE = 5

TESTS = [
    ("strips", "astar(add())", EXIT_PLAN_FOUND),
    ("strips", "astar(hm())", EXIT_PLAN_FOUND),
    ("strips", "ehc(hm())", EXIT_PLAN_FOUND),
    ("strips", "astar(ipdb())", EXIT_PLAN_FOUND),
    ("strips", "astar(lmcut())", EXIT_PLAN_FOUND),
    ("strips", "astar(lmcount(lm_rhw(), admissible=false))", EXIT_PLAN_FOUND),
    ("strips", "astar(lmcount(lm_rhw(), admissible=true))", EXIT_PLAN_FOUND),
    ("strips", "astar(lmcount(lm_hm(), admissible=false))", EXIT_PLAN_FOUND),
    ("strips", "astar(lmcount(lm_hm(), admissible=true))", EXIT_PLAN_FOUND),
    ("strips", "astar(merge_and_shrink())", EXIT_PLAN_FOUND),
    ("axioms", "astar(add())", EXIT_PLAN_FOUND),
    ("axioms", "astar(hm())", EXIT_UNSOLVED_INCOMPLETE),
    ("axioms", "ehc(hm())", EXIT_UNSOLVED_INCOMPLETE),
    ("axioms", "astar(ipdb())", EXIT_UNSUPPORTED),
    ("axioms", "astar(lmcut())", EXIT_UNSUPPORTED),
    ("axioms", "astar(lmcount(lm_rhw(), admissible=false))", EXIT_PLAN_FOUND),
    ("axioms", "astar(lmcount(lm_rhw(), admissible=true))", EXIT_UNSUPPORTED),
    ("axioms", "astar(lmcount(lm_zg(), admissible=false))", EXIT_PLAN_FOUND),
    ("axioms", "astar(lmcount(lm_zg(), admissible=true))", EXIT_UNSUPPORTED),
    # h^m landmark factory explicitly forbids axioms.
    ("axioms", "astar(lmcount(lm_hm(), admissible=false))", EXIT_UNSUPPORTED),
    ("axioms", "astar(lmcount(lm_hm(), admissible=true))", EXIT_UNSUPPORTED),
    ("axioms", "astar(lmcount(lm_exhaust(), admissible=false))", EXIT_PLAN_FOUND),
    ("axioms", "astar(lmcount(lm_exhaust(), admissible=true))", EXIT_UNSUPPORTED),
    ("axioms", "astar(merge_and_shrink())", EXIT_UNSUPPORTED),
    ("cond-eff", "astar(add())", EXIT_PLAN_FOUND),
    ("cond-eff", "astar(hm())", EXIT_PLAN_FOUND),
    ("cond-eff", "astar(ipdb())", EXIT_UNSUPPORTED),
    ("cond-eff", "astar(lmcut())", EXIT_UNSUPPORTED),
    ("cond-eff", "astar(lmcount(lm_rhw(), admissible=false))", EXIT_PLAN_FOUND),
    ("cond-eff", "astar(lmcount(lm_rhw(), admissible=true))", EXIT_PLAN_FOUND),
    ("cond-eff", "astar(lmcount(lm_zg(), admissible=false))", EXIT_PLAN_FOUND),
    ("cond-eff", "astar(lmcount(lm_zg(), admissible=true))", EXIT_PLAN_FOUND),
    ("cond-eff", "astar(lmcount(lm_hm(), admissible=false))", EXIT_PLAN_FOUND),
    ("cond-eff", "astar(lmcount(lm_hm(), admissible=true))", EXIT_UNSUPPORTED),
    ("cond-eff", "astar(lmcount(lm_exhaust(), admissible=false))", EXIT_PLAN_FOUND),
    ("cond-eff", "astar(lmcount(lm_exhaust(), admissible=true))", EXIT_UNSUPPORTED),
    ("cond-eff", "astar(merge_and_shrink())", EXIT_PLAN_FOUND),
]


def run_plan_script(task_type, relpath, search):
    problem = os.path.join(REPO_BASE, "benchmarks", relpath)
    print "\nRun %(search)s on %(task_type)s task:" % locals()
    return subprocess.call(
        [os.path.join(REPO_BASE, "src", "plan"), problem, "--search", search])


def cleanup():
    subprocess.check_call([os.path.join(REPO_BASE, "src", "cleanup")])


def main():
    subprocess.check_call(["./build_all"], cwd=os.path.join(REPO_BASE, "src"))
    failures = []
    for task_type, search, expected in TESTS:
        relpath = TASKS[task_type]
        exitcode = run_plan_script(task_type, relpath, search)
        if not exitcode == expected:
            failures.append((task_type, search, expected, exitcode))
    cleanup()

    if failures:
        print "\nFailures:"
        for task_type, search, expected, exitcode in failures:
            print ("%(search)s on %(task_type)s task: expected %(expected)d, "
                   "got %(exitcode)d" % locals())
        sys.exit(1)
    else:
        print "\nNo errors detected."


main()
