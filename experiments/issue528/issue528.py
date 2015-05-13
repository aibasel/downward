#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import configs, suites
from downward.reports.scatter import ScatterPlotReport

import common_setup


SEARCH_REVS = ["issue528-base", "issue528-v1", "issue528-v2"]
SUITE = suites.suite_optimal_with_ipc11()

CONFIGS = {
    "astar_lmcut": ["--search", "astar(lmcut())"]
}

exp = common_setup.IssueExperiment(
    revisions=SEARCH_REVS,
    configs=CONFIGS,
    suite=SUITE,
    )

exp.add_absolute_report_step()
exp.add_comparison_table_step()

for attr in ("memory", "total_time"):
    exp.add_report(
        ScatterPlotReport(
            attributes=[attr],
            filter_config=[
                "issue528-base-astar_lmcut",
                "issue528-v2-astar_lmcut",
            ],
        ),
        outfile='issue528_base_v2_%s.png' % attr
    )

exp()
