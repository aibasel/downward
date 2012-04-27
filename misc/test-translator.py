#! /usr/bin/env python

"""
This script is meant for testing that the translator works on a specific python
version. Call it with the python version you want to test.

# Run small test on a few problems.
python2.7 test-translator.py

# Run bigger test on the first problem of each domain.
python3.2 test-translator.py first

# Run test on specific problems.
python2.6 test-translator.py gripper:prob01.pddl depot:pfile1
"""

from __future__ import print_function

import os
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TRANSLATOR = os.path.abspath(os.path.join(REPO, 'src/translate'))
BENCHMARKS = os.path.abspath(os.path.join(REPO, 'benchmarks'))

sys.path.insert(0, TRANSLATOR)

import translate
import pddl
import timers


# Translating those problems covers 84% of the translator code. Translating all
# first problems covers 85%.
TASKS = [
    'airport:p01-airport1-p1.pddl',
    'airport-adl:p01-airport1-p1.pddl',
    'assembly:prob01.pddl',
    'barman-opt11-strips:pfile01-001.pddl',
    'miconic-fulladl:f1-0.pddl',
    'psr-large:p01-s29-n2-l5-f30.pddl',
    'transport-opt08-strips:p01.pddl',
    'woodworking-opt11-strips:p01.pddl',
    ]


def translate_task(task_file):
    timer = timers.Timer()
    with timers.timing("Parsing"):
        task = pddl.open(task_file)

    sas_task = translate.pddl_to_sas(task)
    translate.dump_statistics(sas_task)

    with timers.timing("Writing output"):
        sas_task.output(open("output.sas", "w"))
    print("Done! %s" % timer)


def get_first_tasks():
    tasks = []
    for domain in sorted(os.listdir(BENCHMARKS)):
        path = os.path.join(BENCHMARKS, domain)
        domain = [os.path.join(BENCHMARKS, domain, f)
                 for f in sorted(os.listdir(path)) if not 'domain' in f]
        tasks.append(domain[0])
    return tasks


def get_tasks():
    if 'first' in sys.argv:
        # Use the first problem of each domain.
        return get_first_tasks()

    if len(sys.argv) > 1:
        # Use the problems given on the commandline.
        tasks = sys.argv[1:]
    else:
        # Use predefined problems.
        tasks = TASKS

    # Make the paths absolute.
    return [os.path.join(BENCHMARKS, *task.split(':')) for task in tasks]


def main():
    for task in get_tasks():
        print('\nTranslating %s:' % task)
        translate_task(task)


if __name__ == '__main__':
    main()
