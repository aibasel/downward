#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import configs, suites

import common_setup


SEARCH_REVS = ["issue439-base", "issue439-v1"]
LIMITS = {"search_time": 300}
SUITE = suites.suite_satisficing_with_ipc11()

configs_satisficing_core = configs.configs_satisficing_core()
CONFIGS = {}
for name in ["eager_greedy_add", "eager_greedy_ff",
             "lazy_greedy_add", "lazy_greedy_ff"]:
    CONFIGS[name] = configs_satisficing_core[name]


exp = common_setup.IssueExperiment(
    revisions=SEARCH_REVS,
    configs=CONFIGS,
    suite=SUITE,
    limits=LIMITS,
    )

exp.add_absolute_report_step()
exp.add_comparison_table_step()
exp.add_report(common_setup.RegressionReport(
    revision_nicks=exp.revision_nicks,
    config_nicks=CONFIGS.keys()))

exp()
