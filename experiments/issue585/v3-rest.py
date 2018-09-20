#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites
from downward.experiment import FastDownwardExperiment
from downward.reports.compare import CompareConfigsReport

from relativescatter import RelativeScatterPlotReport
import common_setup

REPO = common_setup.get_repo_base()
REV_BASE = 'issue585-base'
REV_V3 = 'issue585-v3'
SUITE = suites.suite_optimal_with_ipc11()
ALGORITHMS = {
    'astar_pdb_base': (REV_BASE, ['--search', 'astar(pdb())']),
    'astar_pdb_v3': (REV_V3, ['--search', 'astar(pdb())']),

    'astar_cpdbs_base': (REV_BASE, ['--search', 'astar(cpdbs())']),
    'astar_cpdbs_v3': (REV_V3, ['--search', 'astar(cpdbs())']),

    'astar_cpdbs_systematic_base': (REV_BASE, ['--search', 'astar(cpdbs_systematic())']),
    'astar_cpdbs_systematic_v3': (REV_V3, ['--search', 'astar(cpdbs(patterns=systematic()))']),

    'astar_zopdbs_base': (REV_BASE, ['--search', 'astar(zopdbs())']),
    'astar_zopdbs_v3': (REV_V3, ['--search', 'astar(zopdbs())']),

    'astar_pho_systematic_base': (REV_BASE, ['--search', 'astar(operatorcounting([pho_constraints_systematic()]))']),
    'astar_pho_systematic_v3': (REV_V3, ['--search', 'astar(operatorcounting([pho_constraints(patterns=systematic())]))']),
}
COMPARED_ALGORITHMS = [
    ('astar_pdb_base', 'astar_pdb_v3', 'Diff (pdb)'),
    ('astar_cpdbs_base', 'astar_cpdbs_v3', 'Diff (cpdbs)'),
    ('astar_cpdbs_systematic_base', 'astar_cpdbs_systematic_v3', 'Diff (cpdbs_systematic)'),
    ('astar_zopdbs_base', 'astar_zopdbs_v3', 'Diff (zopdbs)'),
    ('astar_pho_systematic_base', 'astar_pho_systematic_v3', 'Diff (pho_systematic)'),
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

for c1, c2, _ in COMPARED_ALGORITHMS:
    exp.add_report(
        RelativeScatterPlotReport(
            attributes=["total_time"],
            filter_config=[c1, c2],
            get_category=lambda run1, run2: run1.get("domain"),
        ),
        outfile='issue585_%s_v3_total_time.png' % c1
    )

exp()
