#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os

from downward import suites

import common_setup


DIR = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(DIR))


REVS = ["issue414-base", "issue414"]
LIMITS = {"search_time": 1800}
SUITE = suites.suite_satisficing_with_ipc11()

# The aliases are adjusted for the respective driver scripts by lab.
CONFIGS = {
    "seq_sat_lama_2011": ["ipc", "seq-sat-lama-2011"],
    "seq_sat_fdss_1": ["ipc", "seq-sat-fdss-1"],
    "seq_sat_fdss_2": ["--alias", "seq-sat-fdss-2"],
    "lazy_greedy_ff": [
        "--heuristic", "h=ff()",
        "--search", "lazy_greedy(h, preferred=h)"],
}

exp = common_setup.IssueExperiment(
    revisions=REVS,
    configs=CONFIGS,
    suite=SUITE,
    limits=LIMITS,
    )

exp.add_comparison_table_step()

exp()
