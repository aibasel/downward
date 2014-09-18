#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites

import common_setup

CONFIGS = {
    'astar_lmcount_lm_merged_rhw_hm': [
        '--search',
        'astar(lmcount(lm_merged([lm_rhw(),lm_hm(m=1)]),admissible=true),mpd=true)'],
    'seq_opt_fdss_1': ['ipc', 'seq-opt-fdss-1'],
    'seq_opt_fdss_2': ['ipc', 'seq-opt-fdss-2'],
    'seq_opt_bjolp': ['ipc', 'seq-opt-bjolp'],
}

exp = common_setup.IssueExperiment(
    search_revisions=["issue468-base", "issue468-v1"],
    configs=CONFIGS,
    suite=suites.suite_optimal_with_ipc11(),
    )


def add_first_run_search_time(run):
    if  "search_time" not in run and run.get("search_time_all", []):
        run["search_time"] = run["search_time_all"][0]
    return run

exp.add_comparison_table_step(
    filter=add_first_run_search_time
)

exp()
