#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment

import common_setup
from common_setup import IssueConfig, IssueExperiment
from relativescatter import RelativeScatterPlotReport

DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue739-v2"]
CONFIGS = [
    IssueConfig('translate', [], driver_options=['--translate']),
    IssueConfig('translate-time-limit', [], driver_options=['--translate-time-limit', '5s', '--translate']),
    IssueConfig('translate-memory-limit', [], driver_options=['--translate-memory-limit', '100M', '--translate']),
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
