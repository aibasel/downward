#! /usr/bin/env python3

import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from downward.reports.compare import ComparativeReport

import common_setup
from common_setup import IssueConfig, IssueExperiment

DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPT_NAME = os.path.splitext(os.path.basename(__file__))[0]
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue983-v1", "issue983-v2"]
CONFIGS = []

driver_options = ["--overall-time-limit", "5m"]
for integer in [True, False]:
    for time in [True, False]:
        CONFIGS += [
            IssueConfig(f"opcount-hplus{'-int' if integer else ''}"
                        f"{'-time' if time else ''}",
                        ["--search",
                         "astar(operatorcounting([delete_relaxation_constraints"
                         f"(use_time_vars={time}, use_integer_vars={integer})],"
                         f"use_integer_operator_counts={integer}))"],
                        driver_options=driver_options),
            IssueConfig(f"opcount-hplus-lmcut{'-int' if integer else ''}"
                        f"{'-time' if time else ''}",
                        ["--search",
                         "astar(operatorcounting(["
                         f"delete_relaxation_constraints(use_time_vars={time}, "
                         f"use_integer_vars={integer}), lmcut_constraints], "
                         f"use_integer_operator_counts={integer}))"],
                        driver_options=driver_options)
        ]
    CONFIGS += [
        IssueConfig(f"state-eq{'-int' if integer else ''}",
                    ["--search",
                     "astar(operatorcounting([state_equation_constraints],"
                     f"use_integer_operator_counts={integer}))"],
                    driver_options=driver_options),
        IssueConfig(f"state-eq-lmcut{'-int' if integer else ''}",
                    ["--search",
                     "astar(operatorcounting([state_equation_constraints,"
                     "lmcut_constraints],"
                     f"use_integer_operator_counts={integer}))"],
                    driver_options=driver_options),
        IssueConfig(f"post-hoc-opt{'-int' if integer else ''}",
                    ["--search",
                     "astar(operatorcounting([pho_constraints(systematic(2))],"
                     f"use_integer_operator_counts={integer}))"],
                    driver_options=driver_options),
    ]

SUITE = common_setup.DEFAULT_OPTIMAL_SUITE
ENVIRONMENT = BaselSlurmEnvironment(
    partition="infai_2",
    email="clemens.buechner@unibas.ch",
    export=["PATH", "DOWNWARD_BENCHMARKS"],
)

if common_setup.is_test_run():
    SUITE = IssueExperiment.DEFAULT_TEST_SUITE
    ENVIRONMENT = LocalEnvironment(processes=3)

exp = IssueExperiment(
    revisions=REVISIONS,
    configs=CONFIGS,
    environment=ENVIRONMENT,
)
exp.add_algorithm(
    "relaxed-lmcut", common_setup.get_repo_base(), REVISIONS[0],
    ["--search", "astar(lmcut())", "--translate-options", "--relaxed"])

exp.add_suite(BENCHMARKS_DIR, SUITE)

exp.add_parser(exp.EXITCODE_PARSER)
exp.add_parser(exp.SINGLE_SEARCH_PARSER)
exp.add_parser(exp.PLANNER_PARSER)

exp.add_step('build', exp.build)
exp.add_step('start', exp.start_runs)
exp.add_fetcher(name='fetch')


def get_task(id):
    return f"{id[1]}:{id[2]}"

def add_hplus(run):
    if "hplus" in run["algorithm"]:
        run["hplus"] = run.get("initial_h_value")
    elif "relaxed-lmcut" in run["algorithm"]:
        run["hplus"] = run.get("cost")
    return run


exp.add_report(ComparativeReport(
    filter=[add_hplus],
    attributes=["hplus"],
    algorithm_pairs=[("issue983-v1-opcount-hplus-int-time", "relaxed-lmcut"), ("issue983-v2-opcount-hplus-int-time", "relaxed-lmcut")]),
    outfile="issue983-v2-compare-hplus.html")


exp.add_comparison_table_step(
    attributes=exp.DEFAULT_TABLE_ATTRIBUTES + [
        "initial_h_value"
    ]
)

exp.run_steps()
