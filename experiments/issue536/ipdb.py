#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites

import common_setup


REVS = ["issue536-base", "issue536-v1", "issue536-v2"]
LIMITS = {"search_time": 1800}
SUITE = suites.suite_optimal_with_ipc11()

CONFIGS = {
    "ipdb": ["--search", "astar(ipdb())"],
}

exp = common_setup.IssueExperiment(
    search_revisions=REVS,
    configs=CONFIGS,
    suite=SUITE,
    limits=LIMITS,
    )
exp.add_absolute_report_step()

exp()
