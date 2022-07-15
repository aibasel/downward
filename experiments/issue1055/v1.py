#! /usr/bin/env python3
# -*- coding: utf-8 -*-

import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from lab import tools

from downward.reports.compare import ComparativeReport
from downward.reports import PlanningReport
from downward.experiment import FastDownwardExperiment

import common_setup
from common_setup import IssueConfig, IssueExperiment


DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = [f"issue1055-{version}" for version in ["base", "v1"]]
CONFIGS = [
    IssueConfig(
        "translate-only",
        [],
        driver_options=["--translate"])
]

if common_setup.is_test_run():
    SUITE = IssueExperiment.DEFAULT_TEST_SUITE
    ENVIRONMENT = LocalEnvironment(processes=1)
else:
    SUITE = common_setup.ALL_SUITE
    ENVIRONMENT = BaselSlurmEnvironment(
        partition="infai_2",
        email="patrick.ferber@unibas.ch",
        export=["PATH", "DOWNWARD_BENCHMARKS"])

ATTRIBUTES = [
    'translator_time_done'
]

exp = IssueExperiment(
    revisions=REVISIONS,
    configs=CONFIGS,
    environment=ENVIRONMENT,
)
exp.add_suite(BENCHMARKS_DIR, SUITE)
exp.add_suite(common_setup.get_script_dir(), "negative_example")

exp.add_parser('translator_additional_parser.py')
exp.add_parser(FastDownwardExperiment.TRANSLATOR_PARSER)


class TranslatorDiffReport(PlanningReport):
    def get_cell(self, run):
        return ";".join(run.get(attr) for attr in self.attributes)

    def get_text(self):
        lines = []
        for runs in self.problem_runs.values():
            lhashes = [r.get("translator_output_sas_hash") for r in runs]
            hashes = set(lhashes)
            reason = ""
            if None in hashes:
                reason = f"{len([h for h in lhashes if h is None])} failed + "
            if len(hashes) > 1:
                reason += f"{len([h for h in lhashes if h is not None])} differ"
            if len(reason):
                lines.append(reason + ";" + ";".join([self.get_cell(r) for r in runs]))
        return "\n".join(lines)


exp.add_step('build', exp.build)
exp.add_step('start', exp.start_runs)
exp.add_fetcher(name='fetch')
exp.add_comparison_table_step(attributes=ATTRIBUTES)

exp.add_report(TranslatorDiffReport(
        attributes=["domain", "problem", "algorithm", "run_dir"]
    ), outfile="different_output_sas.csv"
)

exp.run_steps()

