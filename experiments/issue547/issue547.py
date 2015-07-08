#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import configs, suites
from downward.reports.scatter import ScatterPlotReport

import common_setup
from relativescatter import RelativeScatterPlotReport


SEARCH_REVS = ["issue547-base", "issue547-v1"]
SUITE = suites.suite_optimal_with_ipc11()

CONFIGS = {
    'astar_blind': [
        '--search',
        'astar(blind())'],
    'astar_ipdb': [
        '--search',
        'astar(ipdb())'],
    'astar_lmcut': [
        '--search',
        'astar(lmcut())'],
    'astar_pdb': [
        '--search',
        'astar(pdb())'],
}

exp = common_setup.IssueExperiment(
    revisions=SEARCH_REVS,
    configs=CONFIGS,
    suite=SUITE,
    )
exp.add_search_parser("custom-parser.py")

attributes = attributes=exp.DEFAULT_TABLE_ATTRIBUTES + ["successor_generator_time"]
exp.add_comparison_table_step(attributes=attributes)

for conf in CONFIGS:
    for attr in ("memory", "total_time"):
        exp.add_report(
            RelativeScatterPlotReport(
                attributes=[attr],
                get_category=lambda run1, run2: run1.get("domain"),
                filter_config=["issue547-base-%s" % conf, "issue547-v1-%s" % conf]
            ),
            outfile='issue547_base_v1_%s_%s.png' % (conf, attr)
        )

exp()
