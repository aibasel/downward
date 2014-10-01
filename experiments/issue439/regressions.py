#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import configs

import common_setup


SEARCH_REVS = ["issue439-base", "issue439-v1"]
LIMITS = {"search_time": 1800}
SUITE = [
    "airport:p45-airport5MUC-p6.pddl",
    "elevators-sat08-strips:p22.pddl",
    "parking-sat11-strips:pfile09-033.pddl",
    "scanalyzer-08-strips:p30.pddl",
    "transport-sat11-strips:p14.pddl",
    "transport-sat11-strips:p16.pddl",
    "trucks:p19.pddl",
    "trucks-strips:p23.pddl",
]

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
exp.add_search_parser("custom-parser.py")

attributes = attributes=exp.DEFAULT_TABLE_ATTRIBUTES + ["init_time"]
exp.add_absolute_report_step(attributes=attributes)
exp.add_comparison_table_step(attributes=attributes)
exp.add_report(common_setup.RegressionReport(
    revision_nicks=exp.revision_nicks,
    config_nicks=CONFIGS.keys(),
    attributes=attributes))

exp()
