#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites

import common_setup

CONFIGS = {
    'seq_sat_lama_2011': ['ipc', 'seq-sat-lama-2011'],
    'seq_sat_fdss_1': ['ipc', 'seq-sat-fdss-1'],
    'seq_sat_fdss_2': ['ipc', 'seq-sat-fdss-2'],
}

exp = common_setup.IssueExperiment(
    search_revisions=["issue468-base", "issue468-v1"],
    configs=CONFIGS,
    suite=suites.suite_satisficing_with_ipc11(),
    )


def add_first_run_search_time(run):
    if  "search_time" not in run and run.get("search_time_all", []):
        run["search_time"] = run["search_time_all"][0]
    return run

exp.add_comparison_table_step(
    filter=add_first_run_search_time
)

exp()
