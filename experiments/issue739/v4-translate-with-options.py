#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment

import common_setup
from common_setup import IssueConfig, IssueExperiment
from relativescatter import RelativeScatterPlotReport

DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue739-v4"]
CONFIGS = [
    IssueConfig('translate-with-options', ['--translate-options', '--keep-unreachable-facts', '--keep-unimportant-variables', '--full-encoding'], driver_options=['--translate']),
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
del exp.commands['parse-search']
exp.add_absolute_report_step(attributes=['translator_*', 'error'])

exp.run_steps()
