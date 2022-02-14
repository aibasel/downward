#! /usr/bin/env python
# -*- coding: utf-8 -*-

import itertools
import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment

from downward.reports.compare import ComparativeReport
from downward.reports.absolute import AbsoluteReport

import common_setup
from common_setup import IssueConfig, IssueExperiment
from relativescatter import RelativeScatterPlotReport
from derived_variables_instances import DerivedVariableInstances, DERIVED_VARIABLES_SUITE

DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPT_NAME = os.path.splitext(os.path.basename(__file__))[0]
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["tip"]
CONFIGS = []
# Add heuristics using axioms
HEURISTICS = ['ff', 'blind']
LAYER_STRATEGY = ['max', 'min']
OVERAPPROXIMATE = ['none','cycles','all']
KEEP_REDUNDANT_POSITIVE_AXIOMS = [True, False]
NECESSARY_LITERALS = ['exact', 'non-derived', 'positive']

for h in HEURISTICS:
    for ls in LAYER_STRATEGY:
        for overapprox in OVERAPPROXIMATE:
            for rd in KEEP_REDUNDANT_POSITIVE_AXIOMS:
                for lit in NECESSARY_LITERALS:
                    options = ["--evaluator", "heur=%s" % h, "--search", "lazy_greedy([heur], preferred=[heur])", "--translate-options"]
                    options += ["--layer_strategy", ls]
                    options += ["--overapproximate_negated_axioms", overapprox]
                    options += ["--overapproximate_necessary_literals", lit]
                    name = "%s-%s-%s-%s" % (h,ls,overapprox,lit)
                    if rd:
                        options += ["--keep_redundant_positive_axioms"]
                        name += '-kr'
                    CONFIGS.append(IssueConfig(name, options))
#for h in HEURISTICS:
#    CONFIGS.append(IssueConfig(h+"-min-layers", ["--evaluator", "heur=%s" % h,
#                                                                  "--search", "lazy_greedy([heur], preferred=[heur])",
#                                                                  "--translate-options", "--layer_strategy", "min"]),)    
#    CONFIGS.append(IssueConfig(h+"-max-layers", ["--evaluator", "heur=%s" % h,
#                                                                  "--search", "lazy_greedy([heur], preferred=[heur])",
#                                                                  "--translate-options", "--layer_strategy", "max"]),)
# Add A* with blind
#CONFIGS.append(IssueConfig("blind-min-layers", ["--search", "astar(blind)",
#                                                                 "--translate-options", "--layer_strategy", "min"]),)
#CONFIGS.append(IssueConfig("blind-max-layers", ["--search", "astar(blind)",
#                                                                 "--translate-options", "--layer_strategy", "max"]),)

#SUITE = ["psr-middle:p01-s17-n2-l2-f30.pddl"]
SUITE = DERIVED_VARIABLES_SUITE
ENVIRONMENT = BaselSlurmEnvironment(
    partition="infai_2",
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
               'total_time',])

def get_keep_redundant_pairs():
   pairs = []
   for h in HEURISTICS:
      for ls in LAYER_STRATEGY:
          for overapprox in OVERAPPROXIMATE:
              for lit in NECESSARY_LITERALS:
                   pairs.append(("tip-%s-%s-%s-%s" % (h,ls,overapprox,lit), "tip-%s-%s-%s-%s-kr" % (h,ls, overapprox,lit)))
   return pairs
 
exp.add_absolute_report_step(attributes=attributes)
exp.add_report(ComparativeReport(get_keep_redundant_pairs(), attributes=attributes), outfile="issue453-v2-compare_keep_redundant.html")

exp.run_steps()
