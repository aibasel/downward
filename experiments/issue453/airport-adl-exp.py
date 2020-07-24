#! /usr/bin/env python
# -*- coding: utf-8 -*-

import itertools
import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment

from downward.reports.compare import ComparativeReport

import common_setup
from common_setup import IssueConfig, IssueExperiment
from relativescatter import RelativeScatterPlotReport
from derived_variables_instances import DerivedVariableInstances, DERIVED_VARIABLES_SUITE

DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPT_NAME = os.path.splitext(os.path.basename(__file__))[0]
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue453-v1"]
CONFIGS = []
# Add heuristics using axioms
HEURISTICS = ['ff', 'cg', 'cea', 'add']
for h in HEURISTICS:
    CONFIGS.append(IssueConfig(h+"-normal-axiom-rules", ["--evaluator", "heur=%s" % h,
                                                         "--search", "lazy_greedy([heur], preferred=[heur])"]))
    CONFIGS.append(IssueConfig(h+"-overapprox-axiom-rules", ["--evaluator", "heur=%s" % h,
                                                                  "--search", "lazy_greedy([heur], preferred=[heur])",
                                                                  "--translate-options", "--overapproximate-axioms"]),)
# Add lama-first
CONFIGS.append(IssueConfig("lama-normal-axiom-rules", [], driver_options=["--alias", "lama-first"]))
CONFIGS.append(IssueConfig("lama-overapprox-axiom-rules", ["--translate-options", "--overapproximate-axioms"],
                           driver_options=["--alias", "lama-first"]),)
# Add A* with blind
CONFIGS.append(IssueConfig("blind-normal-axiom-rules", ["--search", "astar(blind)"]))
CONFIGS.append(IssueConfig("blind-overapprox-axiom-rules", ["--search", "astar(blind)",
                                                                 "--translate-options", "--overapproximate-axioms"]),)

SUITE = ["airport-adl"]
ENVIRONMENT = BaselSlurmEnvironment(
    partition="infai_1",
    export=["PATH", "DOWNWARD_BENCHMARKS"])

if common_setup.is_test_run():
    SUITE = ["depot:p01.pddl", "gripper:prob01.pddl", "psr-middle:p01-s17-n2-l2-f30.pddl"]
    ENVIRONMENT = LocalEnvironment(processes=1)

exp = IssueExperiment(
    revisions=REVISIONS,
    configs=CONFIGS,
    environment=ENVIRONMENT,
)
exp.add_suite(BENCHMARKS_DIR, SUITE)

exp.add_parser(exp.EXITCODE_PARSER)
exp.add_parser(exp.TRANSLATOR_PARSER)
exp.add_parser(exp.SINGLE_SEARCH_PARSER)
exp.add_parser(exp.PLANNER_PARSER)
exp.add_parser("parser.py")

exp.add_step('build', exp.build)
exp.add_step('start', exp.start_runs)
exp.add_fetcher(name='fetch')

attributes = (['translator_axioms',
               'translator_derived_variables',
               'translator_axioms_removed',
               'translator_time_done',
               'translator_time_processing_axioms',
               'cost',
               'coverage',
               'error',
               'evaluations',
               'expansions',
               'initial_h_value',
               'generated',
               'memory',
               'planner_memory',
               'planner_time',
               'run_dir',
               'search_time',
               'total_time',])
exp.add_absolute_report_step(attributes=attributes)
#exp.add_report(DerivedVariableInstances())

exp.run_steps()
