#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment

import common_setup
from common_setup import IssueConfig, IssueExperiment
from relativescatter import RelativeScatterPlotReport


DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue700-base", "issue700-v1"]
CONFIGS = [
    IssueConfig(
        "lama-first",
        [],
        driver_options=["--alias", "lama-first"]),
    IssueConfig(
        "lama",
        [],
        driver_options=["--alias", "seq-sat-lama-2011"]),
    IssueConfig("ehc_ff", ["--heuristic", "h=ff()", "--search", "ehc(h, preferred=[h])"]),
]
SUITE = common_setup.DEFAULT_SATISFICING_SUITE
ENVIRONMENT = BaselSlurmEnvironment(
    email="florian.pommerening@unibas.ch",
    export=["PATH", "DOWNWARD_BENCHMARKS"])

if common_setup.is_test_run():
    SUITE = IssueExperiment.DEFAULT_TEST_SUITE
    ENVIRONMENT = LocalEnvironment(processes=1)

exp = IssueExperiment(
    revisions=REVISIONS,
    configs=CONFIGS,
    environment=ENVIRONMENT,
)
exp.add_suite(BENCHMARKS_DIR, SUITE)

exp.add_absolute_report_step(filter_algorithm=["lama-first"])
exp.add_comparison_table_step()

for attr in ["total_time", "search_time", "memory"]:
    for rev1, rev2 in [("base", "v1")]:
        for config_nick in ["lama-first", "ehc_ff"]:
            exp.add_report(RelativeScatterPlotReport(
                attributes=[attr],
                filter_algorithm=["issue700-%s-%s" % (rev1, config_nick),
                                  "issue700-%s-%s" % (rev2, config_nick)],
                get_category=lambda r1, r2: r1["domain"],
            ),
            outfile="issue700-%s-%s-%s-%s.png" % (config_nick, attr, rev1, rev2))


exp.run_steps()
