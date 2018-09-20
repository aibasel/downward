#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites
from downward.experiment import FastDownwardExperiment
from downward.reports.compare import CompareConfigsReport

import common_setup

REPO = common_setup.get_repo_base()
REV_BASE = 'issue585-base'
REV_V3 = 'issue585-v3'
SUITE = suites.suite_optimal_with_ipc11()
ALGORITHMS = {
    'astar_cpdbs_genetic': (REV_V3, ['--search', 'astar(cpdbs(patterns=genetic()))']),
    'astar_zopdbs_systematic': (REV_V3, ['--search', 'astar(zopdbs(patterns=systematic()))']),
    'astar_zopdbs_hillclimbing': (REV_V3, ['--search', 'astar(zopdbs(patterns=hillclimbing()))']),
    'astar_pho_genetic': (REV_V3, ['--search', 'astar(operatorcounting([pho_constraints(patterns=genetic())]))']),
    'astar_pho_combo': (REV_V3, ['--search', 'astar(operatorcounting([pho_constraints(patterns=combo())]))']),
}

exp = common_setup.IssueExperiment(
    revisions=[],
    configs={},
    suite=SUITE,
)

for nick, (rev, cmd) in ALGORITHMS.items():
    exp.add_algorithm(nick, REPO, rev, cmd)

exp.add_absolute_report_step()

exp()
