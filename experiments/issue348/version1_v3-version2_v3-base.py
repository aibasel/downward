#! /usr/bin/env python3

import itertools
import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from lab.environments import FreiburgSlurmEnvironment

from downward.reports.compare import ComparativeReport

import common_setup
from common_setup import IssueConfig, IssueExperiment

DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPT_NAME = os.path.splitext(os.path.basename(__file__))[0]
BENCHMARKS_DIR = "/home/drexlerd/benchmarks/downward-benchmarks"
REVISIONS = ["issue348-base", "issue348-version1-v3", "issue348-version2-v3"]
CONFIGS = [
    IssueConfig("lama", [], driver_options=["--alias", "lama-first"]),
    IssueConfig("ehc-ff", ["--search", "ehc(ff())"]),
    IssueConfig("ipdb", ["--search", "astar(ipdb())"]),
    #IssueConfig("lmcut", ["--search", "astar(lmcut())"]),
    IssueConfig("blind", ["--search", "astar(blind())"]),
    #IssueConfig("lazy", [
#	"--evaluator",
#	"hff=ff()",
#	"--evaluator",
#	"hcea=cea()",
#	"--search",
#	"lazy_greedy([hff, hcea], preferred=[hff, hcea])"]),
]

ADL_DOMAINS = [
    "assembly",
    "miconic-fulladl",
    "openstacks",
    "openstacks-opt08-adl",
    "optical-telegraphs",
    "philosophers",
    "psr-large",
    "psr-middle",
    "trucks",
]
SUITE = common_setup.DEFAULT_OPTIMAL_SUITE + ADL_DOMAINS
#ENVIRONMENT = BaselSlurmEnvironment(
#    partition="infai_2",
#    email="florian.pommerening@unibas.ch",
#    export=["PATH", "DOWNWARD_BENCHMARKS"])
ENVIRONMENT = FreiburgSlurmEnvironment()

if common_setup.is_test_run():
    SUITE = IssueExperiment.DEFAULT_TEST_SUITE
    ENVIRONMENT = LocalEnvironment(processes=3)

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

exp.add_absolute_report_step()
exp.add_comparison_table_step()
exp.add_scatter_plot_step(relative=True, attributes=["total_time", "memory"])

exp.run_steps()
