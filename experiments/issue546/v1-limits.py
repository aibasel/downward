#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites

import common_setup


REVS = ["issue546-v1"]
LIMITS = {"search_time": 300, "search_memory": 1024}
SUITE = suites.suite_optimal_with_ipc11()

CONFIGS = {
    "blind-fd-limits": ["--search", "astar(blind())"],
    "blind-lab-limits": ["--search", "astar(blind())"],
}

class FastDownwardLimits(common_setup.IssueExperiment):
    def _make_search_runs(self):
        common_setup.IssueExperiment._make_search_runs(self)
        for run in self.runs:
            if "fd-limits" in run.properties["config_nick"]:
                # Move limits to fast-downward.py
                search_args, search_kwargs = run.commands["search"]
                time_limit = search_kwargs["time_limit"]
                mem_limit = search_kwargs["mem_limit"]
                del search_kwargs["time_limit"]
                del search_kwargs["mem_limit"]
                search_args.insert(1, "--search-timeout")
                search_args.insert(2, str(time_limit))
                search_args.insert(3, "--search-memory")
                search_args.insert(4, str(mem_limit))

exp = FastDownwardLimits(
    search_revisions=REVS,
    configs=CONFIGS,
    suite=SUITE,
    limits=LIMITS,
    )
exp.add_absolute_report_step()

exp()
