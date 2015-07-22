#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites

import common_setup


REVS = ["issue560-base", "issue560-v1"]
SUITE = suites.suite_all()

# We are only interested in the preprocessing here and will only run the first steps of the experiment.
CONFIGS = {
    "astar_blind": [
        "--search",
        "astar(blind())"],
}

exp = common_setup.IssueExperiment(
    search_revisions=REVS,
    configs=CONFIGS,
    suite=SUITE,
    )

exp()
