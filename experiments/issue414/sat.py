#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os

from downward import configs, suites

import common_setup


DIR = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(DIR))


REVS = ["issue414-base", "issue414"]
LIMITS = {"search_time": 1800}
SUITE = suites.suite_satisficing_with_ipc11()
config_pool = configs.default_configs_satisficing()
CONFIGS = {}
for name in ["seq_sat_lama_2011", "seq_sat_fdss_1", "seq_sat_fdss_2",
             "lazy_greedy_ff"]:
    CONFIGS[name] = config_pool[name]

exp = common_setup.IssueExperiment(
    revisions=REVS,
    configs=CONFIGS,
    suite=SUITE,
    limits=LIMITS,
    )

exp.add_comparison_table_step()

exp()
