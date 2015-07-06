#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import configs, suites
from downward.reports.scatter import ScatterPlotReport

import common_setup


SEARCH_REVS = ["issue529-v1-base", "issue529-v1"]
SUITE = suites.suite_optimal_with_ipc11()

CONFIGS = {
    'astar_blind': [
        '--search',
        'astar(blind())'],
    'astar_ipdb': [
        '--search',
        'astar(ipdb())'],
    'astar_cpdbs': [
        '--search',
        'astar(cpdbs())'],
    'astar_gapdb': [
        '--search',
        'astar(gapdb())'],
    'astar_pdb': [
        '--search',
        'astar(pdb())'],
    'astar_zopdbs': [
        '--search',
        'astar(zopdbs())'],
    'eager_greedy_cg': [
        '--heuristic',
        'h=cg()',
        '--search',
        'eager_greedy(h, preferred=h)'],
}

exp = common_setup.IssueExperiment(
    revisions=SEARCH_REVS,
    configs=CONFIGS,
    suite=SUITE,
    )

exp.add_absolute_report_step()
exp.add_comparison_table_step()

exp()
