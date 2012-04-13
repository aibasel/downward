#! /usr/bin/env python

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


def translate_task(task_file):
    timer = timers.Timer()
    with timers.timing("Parsing"):
        task = pddl.open_pddl_file(task_file)

    sas_task = translate.pddl_to_sas(task)
    translate.dump_statistics(sas_task)

    with timers.timing("Writing output"):
        sas_task.output(open("output.sas", "w"))
    print("Done! %s" % timer)


def get_tasks():
    for domain in sorted(os.listdir(BENCHMARKS)):
        path = os.path.join(BENCHMARKS, domain)
        tasks = [os.path.join(BENCHMARKS, domain, f)
                 for f in sorted(os.listdir(path)) if not 'domain' in f]
        yield tasks[0]


for task in get_tasks():
    print('\nTranslating %s:' % task)
    translate_task(task)
