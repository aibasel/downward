#! /usr/bin/env python2
# -*- coding: utf-8 -*-

import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from lab import tools

from downward.reports.compare import ComparativeReport

import common_setup
from common_setup import IssueConfig, IssueExperiment


DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue733-base", "issue733-v1"]
PYTHONS = ["python2.7", "python3.5"]
CONFIGS = [
    IssueConfig(
        "{python}".format(**locals()),
        [],
        driver_options=["--translate"])
    for python in PYTHONS
]
SUITE = set(common_setup.DEFAULT_OPTIMAL_SUITE + common_setup.DEFAULT_SATISFICING_SUITE)
BaselSlurmEnvironment.ENVIRONMENT_SETUP = (
    'module purge\n'
    'module load Python/3.5.2-goolf-1.7.20\n'
    'module load matplotlib/1.5.1-goolf-1.7.20-Python-3.5.2\n'
    'PYTHONPATH="%s:$PYTHONPATH"' % tools.get_lab_path())
ENVIRONMENT = BaselSlurmEnvironment(
    priority=0, email="jendrik.seipp@unibas.ch")

if common_setup.is_test_run():
    SUITE = IssueExperiment.DEFAULT_TEST_SUITE
    ENVIRONMENT = LocalEnvironment(processes=1)

class PythonVersionExperiment(IssueExperiment):
    def _add_runs(self):
        IssueExperiment._add_runs(self)
        for run in self.runs:
            python = run.algo.name.split("-")[-1]
            command, kwargs = run.commands["fast-downward"]
            command = [python] + command
            run.commands["fast-downward"] = (command, kwargs)

exp = PythonVersionExperiment(
    revisions=REVISIONS,
    configs=CONFIGS,
    environment=ENVIRONMENT,
)
del exp.commands["parse-search"]
exp.add_suite(BENCHMARKS_DIR, SUITE)

attributes = ["translator_time_done", "translator_peak_memory"]
exp.add_comparison_table_step(attributes=attributes)
compared_configs = [
    ("issue733-v1-python2.7", "issue733-v1-python3.5", "Diff")]
exp.add_report(
    ComparativeReport(compared_configs, attributes=attributes),
    name="compare-python-versions")

exp.run_steps()
