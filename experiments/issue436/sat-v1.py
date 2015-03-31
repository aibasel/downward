#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites
import configs

import common_setup


REVS = ["issue436-base", "issue436-v1"]
LIMITS = {"search_time": 1800}
SUITE = suites.suite_satisficing_with_ipc11()

default_configs_satisficing = configs.default_configs_satisficing(extended=True)
CONFIGS = {}
for name in ['lazy_greedy_add', 'eager_greedy_ff', 'eager_greedy_add', 'lazy_greedy_ff', 'pareto_ff']:
    CONFIGS[name] = default_configs_satisficing[name]
    
exp = common_setup.IssueExperiment(
    search_revisions=REVS,
    configs=CONFIGS,
    suite=SUITE,
    limits=LIMITS,
    )

exp.add_absolute_report_step()
exp.add_comparison_table_step()


exp()
