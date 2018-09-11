#! /usr/bin/env python2
# -*- coding: utf-8 -*-

import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from lab import tools

from downward.reports.compare import ComparativeReport
from downward.reports import PlanningReport

import common_setup
from common_setup import IssueConfig, IssueExperiment


DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue736-base", "issue736-v1"]
CONFIGS = [
    IssueConfig(
        "translate-only",
        [],
        driver_options=["--translate"])
]
SUITE = set(common_setup.DEFAULT_OPTIMAL_SUITE + common_setup.DEFAULT_SATISFICING_SUITE)
ENVIRONMENT = BaselSlurmEnvironment(email="florian.pommerening@unibas.ch")

if common_setup.is_test_run():
    SUITE = IssueExperiment.DEFAULT_TEST_SUITE
    ENVIRONMENT = LocalEnvironment(processes=1)

exp = IssueExperiment(
    revisions=REVISIONS,
    configs=CONFIGS,
    environment=ENVIRONMENT,
)
del exp.commands["parse-search"]
exp.add_suite(BENCHMARKS_DIR, SUITE)

exp.add_resource("translator_additional_parser", "translator_additional_parser.py", dest="translator_additional_parser.py")
exp.add_command("translator_additional_parser", ["{translator_additional_parser}"])

class TranslatorDiffReport(PlanningReport):
    def get_cell(self, run):
        return ";".join(run.get(attr) for attr in self.attributes)

    def get_text(self):
        lines = []
        for runs in self.problem_runs.values():
            hashes = set([r.get("translator_output_sas_xz_hash") for r in runs])
            if len(hashes) > 1 or None in hashes:
                lines.append(";".join([self.get_cell(r) for r in runs]))
        return "\n".join(lines)


exp.add_report(TranslatorDiffReport(
        attributes=["domain", "problem", "algorithm", "run_dir"]
    ), outfile="different_output_sas.csv"
)

exp.run_steps()
