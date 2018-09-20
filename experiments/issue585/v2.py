#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites
from downward.experiment import FastDownwardExperiment
from downward.reports.compare import CompareConfigsReport

import common_setup

REPO = common_setup.get_repo_base()
REV_BASE = 'issue585-base'
REV_V1 = 'issue585-v2'
SUITE = suites.suite_optimal_with_ipc11()
ALGORITHMS = {
    'astar_ipdb_base': (REV_BASE, ['--search', 'astar(ipdb())']),
    'astar_ipdb_v2': (REV_V1, ['--search', 'astar(ipdb())']),
}
COMPARED_ALGORITHMS = [
    ('astar_ipdb_base', 'astar_ipdb_v2', 'Diff (ipdb)'),
]

exp = common_setup.IssueExperiment(
    revisions=[],
    configs={},
    suite=SUITE,
)

for nick, (rev, cmd) in ALGORITHMS.items():
    exp.add_algorithm(nick, REPO, rev, cmd)

exp.add_report(CompareConfigsReport(
    COMPARED_ALGORITHMS,
    attributes=common_setup.IssueExperiment.DEFAULT_TABLE_ATTRIBUTES
))

exp()
