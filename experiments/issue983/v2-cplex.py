#! /usr/bin/env python3

import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment

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


lmcut_costs = {}
def get_lmcut_costs(run):
    if "lmcut" in run["algorithm"]:
        lmcut_costs[get_task(run["id"])] = run["cost"]
    return run


def add_diff_relaxed_cost(run):
    h = run["initial_h_value"]
    diff = h - lmcut_costs[get_task(run["id"])]
    run["diff_relaxed_cost"] = diff

    inadmissible = 0
    not_equal = 0
    if "hplus" in run["algorithm"]:
        inadmissible = diff > 0
        if "-int-time" in run["algorithm"]:
            not_equal = diff != 0
    run["hplus_inadmissible"] = inadmissible
    run["hplus_not_equal_relaxed_cost"] = not_equal

    return run


exp.add_comparison_table_step(
    filter=[get_lmcut_costs, add_diff_relaxed_cost],
    attributes=exp.DEFAULT_TABLE_ATTRIBUTES + [
        "initial_h_value", "diff_relaxed_cost",
        "hplus_not_equal_relaxed_cost", "hplus_inadmissible",
    ]
)

exp.run_steps()
