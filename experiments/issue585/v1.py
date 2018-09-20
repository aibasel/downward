#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites
from downward.experiment import FastDownwardExperiment
from downward.reports.compare import CompareConfigsReport

import common_setup

REPO = common_setup.get_repo_base()
REV_BASE = 'issue585-base'
REV_V1 = 'issue585-v1'
SUITE = ['gripper:prob01.pddl'] # suites.suite_optimal_with_ipc11()
ALGORITHMS = {
	'astar_pdb_base': (REV_BASE, ['--search', 'astar(pdb())']),
	'astar_pdb_v1': (REV_V1, ['--search', 'astar(pdb())']),

	'astar_cpdbs_base': (REV_BASE, ['--search', 'astar(cpdbs())']),
	'astar_cpdbs_v1': (REV_V1, ['--search', 'astar(cpdbs())']),

	'astar_cpdbs_systematic_base': (REV_BASE, ['--search', 'astar(cpdbs_systematic())']),
	'astar_cpdbs_systematic_v1': (REV_V1, ['--search', 'astar(cpdbs(patterns=systematic()))']),

	'astar_zopdbs_base': (REV_BASE, ['--search', 'astar(zopdbs())']),
	'astar_zopdbs_v1': (REV_V1, ['--search', 'astar(zopdbs())']),

	'astar_ipdb_base': (REV_BASE, ['--search', 'astar(ipdb())']),
	'astar_ipdb_v1': (REV_V1, ['--search', 'astar(ipdb())']),
	'astar_ipdb_alias': (REV_V1, ['--search', 'astar(cpdbs(patterns=hillclimbing()))']),

	'astar_gapdb_base': (REV_BASE, ['--search', 'astar(gapdb())']),
	'astar_gapdb_v1': (REV_V1, ['--search', 'astar(zopdbs(patterns=genetic()))']),

	'astar_pho_systematic_base': (REV_BASE, ['--search', 'astar(operatorcounting([pho_constraints_systematic()]))']),
	'astar_pho_systematic_v1': (REV_V1, ['--search', 'astar(operatorcounting([pho_constraints(patterns=systematic())]))']),

	'astar_pho_hillclimbing_base': (REV_BASE, ['--search', 'astar(operatorcounting([pho_constraints_ipdb()]))']),
	'astar_pho_hillclimbing_v1': (REV_V1, ['--search', 'astar(operatorcounting([pho_constraints(patterns=hillclimbing())]))']),
}
COMPARED_ALGORITHMS = [
	('astar_pdb_base', 'astar_pdb_v1', 'Diff (pdb)'),
	('astar_cpdbs_base', 'astar_cpdbs_v1', 'Diff (cpdbs)'),
	('astar_cpdbs_systematic_base', 'astar_cpdbs_systematic_v1', 'Diff (cpdbs_systematic)'),
	('astar_zopdbs_base', 'astar_zopdbs_v1', 'Diff (zopdbs)'),
	('astar_ipdb_base', 'astar_ipdb_v1', 'Diff (ipdb)'),
	('astar_ipdb_v1', 'astar_ipdb_alias', 'Diff (ipdb_alias)'),
	('astar_gapdb_base', 'astar_gapdb_v1', 'Diff (gapdb)'),
	('astar_pho_systematic_base', 'astar_pho_systematic_v1', 'Diff (pho_systematic)'),
	('astar_pho_hillclimbing_base', 'astar_pho_hillclimbing_v1', 'Diff (pho_hillclimbing)'),
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
