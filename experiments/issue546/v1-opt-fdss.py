#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites

import common_setup


REVS = ["issue546-base", "issue546-v1"]
LIMITS = {"search_time": 1800}
SUITE = suites.suite_optimal_with_ipc11()

CONFIGS = {
    "seq_opt_fdss_1": ["--alias", "seq-opt-fdss-1"],
    "seq_opt_fdss_2": ["--alias", "seq-opt-fdss-2"],
}

exp = common_setup.IssueExperiment(
    search_revisions=REVS,
    configs=CONFIGS,
    suite=SUITE,
    limits=LIMITS,
    )
exp.add_comparison_table_step(
    attributes=common_setup.IssueExperiment.PORTFOLIO_ATTRIBUTES)

exp()
