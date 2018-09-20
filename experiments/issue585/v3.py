#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites
from downward.experiment import FastDownwardExperiment
from downward.reports.compare import CompareConfigsReport

from relativescatter import RelativeScatterPlotReport
import common_setup

REPO = common_setup.get_repo_base()
REV_BASE = 'issue585-base'
REV_V1 = 'issue585-v3'
SUITE = suites.suite_optimal_with_ipc11()
ALGORITHMS = {
    'astar_ipdb_base': (REV_BASE, ['--search', 'astar(ipdb())']),
    'astar_ipdb_v3': (REV_V1, ['--search', 'astar(ipdb())']),

    'astar_gapdb_base': (REV_BASE, ['--search', 'astar(gapdb())']),
    'astar_gapdb_v3': (REV_V1, ['--search', 'astar(zopdbs(patterns=genetic()))']),
}
COMPARED_ALGORITHMS = [
    ('astar_ipdb_base', 'astar_ipdb_v3', 'Diff (ipdb)'),
    ('astar_gapdb_base', 'astar_gapdb_v3', 'Diff (gapdb)'),
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

exp.add_report(
    RelativeScatterPlotReport(
        attributes=["total_time"],
        filter_config=["astar_ipdb_base", "astar_ipdb_v3"],
        get_category=lambda run1, run2: run1.get("domain"),
    ),
    outfile='issue585_ipdb_base_v3_total_time.png'
)

exp.add_report(
    RelativeScatterPlotReport(
        attributes=["total_time"],
        filter_config=["astar_gapdb_base", "astar_gapdb_v3"],
        get_category=lambda run1, run2: run1.get("domain"),
    ),
    outfile='issue585_gapdb_base_v3_total_time.png'
)

exp()
