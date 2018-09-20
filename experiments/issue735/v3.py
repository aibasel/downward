#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment

import common_setup
from common_setup import IssueConfig, IssueExperiment
from relativescatter import RelativeScatterPlotReport

DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue735-base", "issue735-v1", "issue735-v2", "issue735-v3"]
BUILD_OPTIONS = ["release32nolp"]
DRIVER_OPTIONS = ["--build", "release32nolp"]
CONFIGS = [
    IssueConfig(
        "cpdbs-sys2",
        ["--search", "astar(cpdbs(systematic(2)))"],
        build_options=BUILD_OPTIONS,
        driver_options=DRIVER_OPTIONS),
    IssueConfig(
        "cpdbs-hc",
        ["--search", "astar(cpdbs(hillclimbing(max_time=900)))"],
        build_options=BUILD_OPTIONS,
        driver_options=DRIVER_OPTIONS),
]
SUITE = common_setup.DEFAULT_OPTIMAL_SUITE
ENVIRONMENT = BaselSlurmEnvironment(
    email="jendrik.seipp@unibas.ch",
    export=["PATH", "DOWNWARD_BENCHMARKS"])

if common_setup.is_test_run():
    SUITE = IssueExperiment.DEFAULT_TEST_SUITE
    ENVIRONMENT = LocalEnvironment(processes=1)

IssueExperiment.DEFAULT_TABLE_ATTRIBUTES += [
    "dominance_pruning_failed",
    "dominance_pruning_time",
    "dominance_pruning_pruned_subsets",
    "dominance_pruning_pruned_pdbs",
    "pdb_collection_construction_time"]

exp = IssueExperiment(
    revisions=REVISIONS,
    configs=CONFIGS,
    environment=ENVIRONMENT,
)
exp.add_algorithm(
    "issue735-v3:cpdbs-sys2-debug",
    common_setup.get_repo_base(),
    "issue735-v3",
    ["--search", "astar(cpdbs(systematic(2)))"],
    build_options=["debug32nolp"],
    driver_options=["--build", "debug32nolp"])
exp.add_resource("custom_parser", "custom-parser.py")
exp.add_command("run-custom-parser", ["{custom_parser}"])
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
