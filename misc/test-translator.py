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
import platform
import re
import shutil
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TRANSLATOR = os.path.abspath(os.path.join(REPO, 'src/translate'))
BENCHMARKS = os.path.abspath(os.path.join(REPO, 'benchmarks'))
SAS_FILES = '/tmp/sas-files'

sys.path.insert(0, TRANSLATOR)

import translate


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
    print('\nTranslating %s:' % task_file)
    sys.argv = [sys.argv[0], '--force-old-python', task_file]
    translate.main()


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

    # Use the problems given on the commandline.
    tasks = [arg for arg in sys.argv[1:] if not arg.startswith('--')]
    if not tasks:
        # Use predefined problems.
        tasks = TASKS

    # Make the paths absolute.
    return [os.path.join(BENCHMARKS, *task.split(':')) for task in tasks]


def get_task_dest(task):
    task_name = '-'.join(task.split('/')[-2:])
    dest = os.path.join(SAS_FILES, task_name, platform.python_version())
    try:
        os.makedirs(os.path.dirname(dest))
    except OSError:
        pass
    return dest


def save_task(task_dest):
    with open('output.sas') as infile:
        with open(task_dest, 'a') as outfile:
            outfile.write(infile.read())


class Logger(object):
    patterns = [r'\[.+s CPU, .+s wall-clock\]', r'\d+ KB', r'at 0x.{7}',
                r'\d+ auxiliary atoms', r'\d+ final queue length',
                r'\d+ total queue pushes']

    def __init__(self, logfile):
        self.logfile = logfile

    def __enter__(self):
        self.log = open(self.logfile, 'w')
        sys.stdout = self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.log.close()
        sys.stdout = sys.__stdout__

    def write(self, text):
        for pattern in self.patterns:
            text = re.sub(pattern, '', text)
        self.log.write(text)

    def flush(self):
        self.log.flush()


def main():
    for task in get_tasks():
        dest = get_task_dest(task)
        with Logger(dest):
            translate_task(task)
        save_task(dest)


if __name__ == '__main__':
    main()
