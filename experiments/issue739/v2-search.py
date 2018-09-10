#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment

import common_setup
from common_setup import IssueConfig, IssueExperiment
from relativescatter import RelativeScatterPlotReport

DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue739-base", "issue739-v2"]
CONFIGS = [
    IssueConfig('search-time-limit', ['--search', 'astar(blind())'], driver_options=['--search-time-limit', '20s']),
    IssueConfig('search-memory-limit', ['--search', 'astar(blind())'], driver_options=['--search-memory-limit', '100M']),
]
SUITE = common_setup.DEFAULT_OPTIMAL_SUITE
ENVIRONMENT = BaselSlurmEnvironment(email="silvan.sievers@unibas.ch", export=["PATH", "DOWNWARD_BENCHMARKS"])

if common_setup.is_test_run():
    SUITE = ['gripper:prob10.pddl','mystery:prob07.pddl']
    ENVIRONMENT = LocalEnvironment(processes=4)

exp = IssueExperiment(
    revisions=REVISIONS,
    configs=CONFIGS,
    environment=ENVIRONMENT,
)
exp.add_suite(BENCHMARKS_DIR, SUITE)
exp.add_resource('exit_code_converter_parser', 'exit-code-converter-parser.py', dest='exit-code-converter-parser.py')
exp.add_command('exit-code-converter-parser', ['{exit_code_converter_parser}'])
exp.add_comparison_table_step()

exp.run_steps()
