#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites

import common_setup

CONFIGS = {
        'astar_gapdb': [
            '--search',
            'astar(gapdb())'],
}

exp = common_setup.IssueExperiment(
    search_revisions=["issue482-base", "issue482-v1"],
    configs=CONFIGS,
    suite=suites.suite_optimal_with_ipc11(),
    )

exp.add_comparison_table_step()

exp()
