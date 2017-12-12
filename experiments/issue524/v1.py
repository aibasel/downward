#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os
import os.path

from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from lab.reports import Attribute, geometric_mean

import common_setup
from common_setup import IssueConfig, IssueExperiment
from relativescatter import RelativeScatterPlotReport

DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISION_CACHE = os.path.expanduser('~/lab/revision-cache')
REVISIONS = ["issue524-base", "issue524-v1"]
CONFIGS = [
    IssueConfig('lm_mh', [
        '--landmarks', 'l=lm_hm(m=1)',
        '--heuristic', 'h=lmcount(l)',
        '--search', 'eager_greedy([h])']),
]
SUITE = common_setup.DEFAULT_OPTIMAL_SUITE
ENVIRONMENT = BaselSlurmEnvironment(
    email="cedric.geissmann@unibas.ch"
)

# SUITE = ['gripper:prob01.pddl', 'rovers:p01.pddl']
SUITE = common_setup.DEFAULT_OPTIMAL_SUITE

if common_setup.is_test_run():
    SUITE = IssueExperiment.DEFAULT_TEST_SUITE
    ENVIRONMENT = LocalEnvironment(processes=4)

exp = IssueExperiment(
    revisions=REVISIONS,
    configs=CONFIGS,
    environment=ENVIRONMENT,
    revision_cache=REVISION_CACHE,
)
exp.add_suite(BENCHMARKS_DIR, SUITE)

exp.add_absolute_report_step()
exp.add_comparison_table_step()

exp.run_steps()
