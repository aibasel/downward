#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment

import common_setup
from common_setup import IssueConfig, IssueExperiment


DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["8e0b8e1f6edc"]
EXPORTS = ["PYTHONPATH", "PATH", "DOWNWARD_BENCHMARKS"]


def generate_configs(sas_filenames):
    configs = []
    for sas_file in sas_filenames:

        common_driver_options = [] if sas_file is None else ["--sas-file", sas_file]

        configs += [
            IssueConfig('lazy-greedy-blind-{}'.format(sas_file), ['--search', 'lazy_greedy([blind()])'],
                        driver_options=common_driver_options + []),

            IssueConfig('lama-first-{}'.format(sas_file), [],
                        driver_options=common_driver_options + ["--alias", "lama-first"]),

            IssueConfig("seq_sat_fdss_1-{}".format(sas_file), [],
                        driver_options=common_driver_options + ["--alias", "seq-sat-fdss-1"]),

            IssueConfig("seq_sat_fdss_-{}".format(sas_file), [],
                        driver_options=common_driver_options + ["--portfolio", "driver/portfolios/seq_sat_fdss_2.py",
                                                              "--overall-time-limit", "20s"]),

            IssueConfig('translate-only-{}'.format(sas_file), [],
                        driver_options=['--translate'] + common_driver_options),
        ]
    return configs


def generate_experiments(configs):
    SUITE = ["gripper:prob01.pddl",
             "blocks:probBLOCKS-5-0.pddl",
             "visitall-sat11-strips:problem12.pddl",
             "airport:p01-airport1-p1.pddl"]

    ENVIRONMENT = BaselSlurmEnvironment(email="guillem.frances@unibas.ch", export=EXPORTS)

    if common_setup.is_test_run():
        SUITE = IssueExperiment.DEFAULT_TEST_SUITE
        ENVIRONMENT = LocalEnvironment(processes=2)

    exp = IssueExperiment(
        revisions=REVISIONS,
        configs=configs,
        environment=ENVIRONMENT,
    )
    exp.add_suite(BENCHMARKS_DIR, SUITE)
    exp.add_absolute_report_step()
    exp.run_steps()
