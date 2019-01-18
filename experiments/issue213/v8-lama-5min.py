#! /usr/bin/env python
# -*- coding: utf-8 -*-

import itertools
import os
import subprocess

from lab.environments import LocalEnvironment, BaselSlurmEnvironment

from downward.reports.compare import ComparativeReport

import common_setup
from common_setup import IssueConfig, IssueExperiment
from relativescatter import RelativeScatterPlotReport

EXPNAME = common_setup.get_experiment_name()
DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue213-v8"]
BUILDS = ["release32", "release64"]
CONFIGS = [
    IssueConfig(
        "lama-" + build,
        [],
        build_options=[build],
        driver_options=["--build", build, "--alias", "lama", "--overall-time-limit", "5m"])
    for rev in REVISIONS
    for build in BUILDS
]
SUITE = common_setup.DEFAULT_SATISFICING_SUITE
ENVIRONMENT = BaselSlurmEnvironment(
    partition="infai_1",
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

exp.add_parser(exp.EXITCODE_PARSER)
exp.add_parser(exp.ANYTIME_SEARCH_PARSER)
exp.add_parser(exp.PLANNER_PARSER)

exp.add_step("build", exp.build)
exp.add_step("start", exp.start_runs)
exp.add_fetcher(name="fetch")

#exp.add_absolute_report_step()
#exp.add_comparison_table_step()

attributes = IssueExperiment.PORTFOLIO_ATTRIBUTES

# Compare builds.
for build1, build2 in itertools.combinations(BUILDS, 2):
    for rev in REVISIONS:
        algorithm_pairs = [
            ("{rev}-{config_nick}-{build1}".format(**locals()),
             "{rev}-{config_nick}-{build2}".format(**locals()),
             "Diff ({config_nick}-{rev})".format(**locals()))
            for config_nick in ["lama"]]
        outfile = os.path.join(exp.eval_dir, "{EXPNAME}-{build1}-vs-{build2}.html".format(**locals()))
        exp.add_report(
            ComparativeReport(algorithm_pairs, attributes=attributes),
            outfile=outfile)
        exp.add_step(
            'publish-report', subprocess.call, ['publish', outfile])

exp.run_steps()
