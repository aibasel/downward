#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment

import common_setup
from common_setup import IssueConfig, IssueExperiment
from relativescatter import RelativeScatterPlotReport

DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue750-base", "issue750-v1"]
BUILD_OPTIONS = ["release32nolp"]
DRIVER_OPTIONS = ["--build", "release32nolp", "--search-time-limit", "1m"]
CONFIGS = [
    IssueConfig(
        "blind",
        ["--search", "astar(blind())"],
        build_options=BUILD_OPTIONS,
        driver_options=DRIVER_OPTIONS),
    IssueConfig(
        "lama-first",
        [],
        build_options=BUILD_OPTIONS,
        driver_options=DRIVER_OPTIONS + ["--alias", "lama-first"]),
]
SUITE = common_setup.DEFAULT_SATISFICING_SUITE
ENVIRONMENT = BaselSlurmEnvironment(
    email="jendrik.seipp@unibas.ch",
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

for attribute in ["total_time"]:
    for config in CONFIGS:
        exp.add_report(
            RelativeScatterPlotReport(
                attributes=[attribute],
                filter_algorithm=["{}-{}".format(rev, config.nick) for rev in REVISIONS],
                get_category=lambda run1, run2: run1.get("domain"),
            ),
            outfile="{}-{}-{}-{}-{}.png".format(exp.name, attribute, config.nick, *REVISIONS)
        )

exp.run_steps()
