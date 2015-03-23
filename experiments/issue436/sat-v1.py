#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites
import configs

import common_setup


REVS = ["issue436-base", "issue436-v1"]
LIMITS = {"search_time": 1800}
SUITE = suites.suite_satisficing_with_ipc11()

configs_satisficing_core = configs.configs_satisficing_core()
CONFIGS = {}
for name in ['lazy_greedy_add', 'eager_greedy_ff', 'eager_greedy_add', 'lazy_greedy_ff', 'pareto_ff']:
    CONFIGS[name] = configs_satisficing_core[name]


exp = common_setup.IssueExperiment(
    search_revisions=REVS,
    configs=CONFIGS,
    suite=SUITE,
    limits=LIMITS,
    )

exp.add_absolute_report_step()
exp.add_comparison_table_step()


exp()
