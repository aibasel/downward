#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites

import common_setup

CONFIGS = {
        'astar_lmcount': [
            '--search',
            'astar(lmcount(lm_merged([lm_rhw(),lm_hm(m=1)]),admissible=true,optimal=true),mpd=true)'],
}

exp = common_setup.IssueExperiment(
    search_revisions=["issue443-base", "issue443-v2"],
    configs=CONFIGS,
    suite=suites.suite_optimal_with_ipc11(),
    )

exp.add_comparison_table_step()

exp()
