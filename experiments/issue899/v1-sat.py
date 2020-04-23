#! /usr/bin/env python3

import itertools
import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment

from downward.reports.compare import ComparativeReport

import common_setup
from common_setup import IssueConfig, IssueExperiment
from relativescatter import RelativeScatterPlotReport

DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPT_NAME = os.path.splitext(os.path.basename(__file__))[0]
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue899-base", "issue899-v1"]
CONFIGS = [
    IssueConfig("lama-first", [], driver_options=["--alias", "lama-first"]),
    IssueConfig("lm_hm", [
        "--landmarks", "lm=lm_hm(2)",
        "--heuristic", "hlm=lmcount(lm)",
        "--search", "lazy_greedy([hlm])"]),
    IssueConfig("lm_exhaust", [
        "--landmarks", "lm=lm_exhaust()",
        "--heuristic", "hlm=lmcount(lm)",
        "--search", "lazy_greedy([hlm])"]),
    IssueConfig("lm_rhw", [
        "--landmarks", "lm=lm_rhw()",
        "--heuristic", "hlm=lmcount(lm)",
        "--search", "lazy_greedy([hlm])"]),
    IssueConfig("lm_zg", [
        "--landmarks", "lm=lm_zg()",
        "--heuristic", "hlm=lmcount(lm)",
        "--search", "lazy_greedy([hlm])"]),
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
exp.add_parser(exp.SINGLE_SEARCH_PARSER)
exp.add_parser(exp.PLANNER_PARSER)

exp.add_step('build', exp.build)
exp.add_step('start', exp.start_runs)
exp.add_fetcher(name='fetch')

#exp.add_absolute_report_step()
exp.add_comparison_table_step()

exp.run_steps()
