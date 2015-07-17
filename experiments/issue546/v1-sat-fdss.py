#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites

import common_setup


REVS = ["issue546-base", "issue546-v1"]
LIMITS = {"search_time": 1800}
SUITE = suites.suite_satisficing_with_ipc11()

CONFIGS = {
    "seq_sat_fdss_1": ["--alias", "seq-sat-fdss-1"],
    "seq_sat_fdss_2": ["--alias", "seq-sat-fdss-2"],
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
