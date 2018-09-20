#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites
from lab.reports import Attribute
from lab.suites import suite_all

import common_setup

import os


exp = common_setup.IssueExperiment(
    search_revisions=["issue469-base", "issue469-v1"],
    configs={"astar_blind": ["--search", "astar(blind())"]},
    suite=suite_all(),
    )

parser = os.path.join(common_setup.get_script_dir(),
                      'raw_memory_parser.py')
exp.add_search_parser(parser)

def add_unexplained_errors_as_int(run):
    if run.get('error').startswith('unexplained'):
        run['unexplained_errors'] = 1
    else:
        run['unexplained_errors'] = 0
    return run

exp.add_absolute_report_step(
    attributes=['raw_memory', Attribute('unexplained_errors', absolute=True)],
    filter=add_unexplained_errors_as_int
)

exp()
