#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites
import configs

import common_setup


REVS = ["issue436-base", "issue436-v2"]
LIMITS = {"search_time": 1800}
SUITE = suites.suite_optimal_with_ipc11()

configs_optimal_core = configs.configs_optimal_core()
CONFIGS = {}
for name in ['astar_merge_and_shrink_greedy_bisim', 'astar_merge_and_shrink_dfp_bisim',
             'astar_ipdb', 'astar_hmax', 'astar_blind', 'astar_lmcut',
             'astar_merge_and_shrink_bisim', 'astar_lmcount_lm_merged_rhw_hm']:
    CONFIGS[name] = configs_optimal_core[name]


exp = common_setup.IssueExperiment(
    search_revisions=REVS,
    configs=CONFIGS,
    suite=SUITE,
    limits=LIMITS,
    )

exp.add_absolute_report_step()
exp.add_comparison_table_step()
exp.add_scatter_plot_step()

exp()
