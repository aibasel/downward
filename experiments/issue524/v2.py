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
REVISIONS = ["issue524-base-v2", "issue524-v2"]
CONFIGS = [
    IssueConfig('lm_hm', [
        '--landmarks', 'l=lm_hm()',
        '--heuristic', 'h=lmcount(l)',
        '--search', 'eager_greedy([h])']),
] + [
    IssueConfig('lm_rhw', [
        '--landmarks', 'l=lm_rhw()',
        '--heuristic', 'h=lmcount(l)',
        '--search', 'eager_greedy([h])']),
] + [
    IssueConfig('lm_zg', [
        '--landmarks', 'l=lm_zg()',
        '--heuristic', 'h=lmcount(l)',
        '--search', 'eager_greedy([h])']),
] + [
    IssueConfig('lm_exhaust', [
        '--landmarks', 'l=lm_exhaust()',
        '--heuristic', 'h=lmcount(l)',
        '--search', 'eager_greedy([h])']),
] + [
    IssueConfig('lm_merged', [
        '--landmarks', 'l1=lm_exhaust()',
        '--landmarks', 'l2=lm_rhw()',
        '--landmarks', 'l=lm_merged([l1, l2])',
        '--heuristic', 'h=lmcount(l)',
        '--search', 'eager_greedy([h])']),
] + [
    IssueConfig(
        "lama-first", [], driver_options=["--alias", "lama-first"])
]
SUITE = common_setup.DEFAULT_OPTIMAL_SUITE
ENVIRONMENT = BaselSlurmEnvironment(
    email="cedric.geissmann@unibas.ch"
)

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
exp.add_scatter_plot_step()

exp.run_steps()
