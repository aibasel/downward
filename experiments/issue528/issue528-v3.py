#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import configs, suites

import common_setup


SEARCH_REVS = ["issue528-base", "issue528-v3"]
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

exp()
