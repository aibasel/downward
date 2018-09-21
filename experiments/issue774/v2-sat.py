#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment

import common_setup
from common_setup import IssueConfig, IssueExperiment
from relativescatter import RelativeScatterPlotReport


DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue774-v2-base", "issue774-v2"]
CONFIGS = [
    IssueConfig("blind", ["--search", "astar(blind())"]),
    IssueConfig("ehc_ff", ["--heuristic", "h=ff()", "--search", "ehc(h, preferred=[h])"]),
    IssueConfig(
        "lama-first",
        [],
        driver_options=["--alias", "lama-first"]),
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

exp.add_absolute_report_step()
exp.add_comparison_table_step()

for attr in ["total_time", "search_time", "memory"]:
    for rev1, rev2 in [("v2-base", "v2")]:
        for config_nick in ["blind", "ehc_ff", "lama-first"]:
            exp.add_report(RelativeScatterPlotReport(
                attributes=[attr],
                filter_algorithm=["issue774-%s-%s" % (rev1, config_nick),
                                  "issue774-%s-%s" % (rev2, config_nick)],
                get_category=lambda r1, r2: r1["domain"],
            ),
            outfile="issue774-%s-%s-%s-%s.png" % (config_nick, attr, rev1, rev2))


exp.run_steps()
