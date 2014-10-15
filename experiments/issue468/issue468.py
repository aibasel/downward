#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites

import common_setup

CONFIGS = {
    'astar_lmcount_lm_merged_rhw_hm': [
        '--search',
        'astar(lmcount(lm_merged([lm_rhw(),lm_hm(m=1)]),admissible=true),mpd=true)'],
}

exp = common_setup.IssueExperiment(
    search_revisions=["issue468-base", "issue468-v1"],
    configs=CONFIGS,
    suite=suites.suite_optimal_with_ipc11(),
    )

exp.add_comparison_table_step()

exp()
