#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites
from downward import configs

import common_setup


REVS = ["issue436-base", "issue436-v1"]
LIMITS = {"search_time": 1800}
SUITE = suites.suite_optimal_with_ipc11()

configs_satisficing_core = configs.configs_satisficing_core()
CONFIGS = {}
for name in ["eager_greedy_add", "eager_greedy_ff",
             "lazy_greedy_add", "lazy_greedy_ff",
             "astar_lmcount_lm_merged_rhw_hm",
             "pareto_lmcut",
             "astar_merge_and_shrink_bisim"]:
    CONFIGS[name] = configs_satisficing_core[name]


exp = common_setup.IssueExperiment(
    search_revisions=REVS,
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
