#! /usr/bin/env python
# -*- coding: utf-8 -*-

import itertools
import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment

from downward.reports.compare import ComparativeReport
from downward.reports.absolute import AbsoluteReport

import common_setup
from common_setup import IssueConfig, IssueExperiment, get_repo_base
from derived_variables_instances import DerivedVariableInstances, DERIVED_VARIABLES_SUITE

DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPT_NAME = os.path.splitext(os.path.basename(__file__))[0]
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
BENCHMARKS_DIR_CUSTOM = "/infai/simsal00/translate-pddls"
SUITE = DERIVED_VARIABLES_SUITE
SUITE_CUSTOM = ["bwnc", "citycar", "cups", "failed-negation", "graph", "layers", "mst"]


CONFIGS = {'blind': (["--search", "astar(blind())"],[],[]),
           'ff-eager': (["--evaluator", "heur=ff", "--search", 
              "eager_greedy([heur], preferred=[heur])"],[],[]),
           'ff-lazy': (["--evaluator", "heur=ff", "--search", 
              "lazy_greedy([heur], preferred=[heur])"],[],[]),
           'lama-first': ([],[],["--alias", "lama-first"]) }

ENVIRONMENT = BaselSlurmEnvironment(
    partition="infai_2",
    export=["PATH", "DOWNWARD_BENCHMARKS"])

if common_setup.is_test_run():
    SUITE = ["depot:p01.pddl", "gripper:prob01.pddl", "psr-middle:p01-s17-n2-l2-f30.pddl"]
    ENVIRONMENT = LocalEnvironment(processes=1)

exp = IssueExperiment(environment=ENVIRONMENT, revisions=[], configs=[])
exp.add_suite(BENCHMARKS_DIR, SUITE)
exp.add_suite(BENCHMARKS_DIR_CUSTOM, SUITE_CUSTOM)
#exp.add_suite(BENCHMARKS_DIR, ["psr-middle:p01-s17-n2-l2-f30.pddl"])

for name, config in CONFIGS.items():
    exp.add_algorithm(name+'-base',get_repo_base(),'issue453-base',config[0],config[1],config[2])
    exp.add_algorithm(name+'-v4',get_repo_base(),'issue453-v4',config[0],config[1],config[2])
    exp.add_algorithm(name+'-v4-max-layers',get_repo_base(),'issue453-v4',config[0]+['--translate-options', '--layer-strategy=max'], config[1],config[2])


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
               'translator_task_size',
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
               'total_time',
               'score_evaluations',
               'score_search_time',
               'score_total_time',
               ]) 
exp.add_absolute_report_step(attributes=attributes)

exp.run_steps()
