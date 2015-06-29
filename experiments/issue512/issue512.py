#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import configs, suites

import common_setup


SEARCH_REVS = ["issue512-base", "issue512-v1"]
LIMITS = {"search_time": 1800}
SUITE = suites.suite_satisficing_with_ipc11()

configs_satisficing_core = configs.configs_satisficing_core()
CONFIGS = {}
for name in ["eager_greedy_add", "eager_greedy_ff",
             "lazy_greedy_add", "lazy_greedy_ff"]:
    CONFIGS[name] = configs_satisficing_core[name]
CONFIGS["blind"] = ["--search", "astar(blind())"]


exp = common_setup.IssueExperiment(
    revisions=SEARCH_REVS,
    configs=CONFIGS,
    suite=SUITE,
    limits=LIMITS,
    )
exp.add_search_parser("custom-parser.py")

attributes = attributes=exp.DEFAULT_TABLE_ATTRIBUTES + ["init_time"]
exp.add_absolute_report_step(attributes=attributes)
exp.add_comparison_table_step(attributes=attributes)
exp.add_report(common_setup.RegressionReport(
    revision_nicks=exp.revision_nicks,
    config_nicks=CONFIGS.keys(),
    attributes=attributes))

exp()
