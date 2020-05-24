#! /usr/bin/env python
# -*- coding: utf-8 -*-

from collections import defaultdict
import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from lab.experiment import Experiment

from downward.reports import PlanningReport
from downward.reports.absolute import AbsoluteReport
from downward.reports.compare import ComparativeReport

import common_setup


DIR = os.path.dirname(os.path.abspath(__file__))
ENVIRONMENT = BaselSlurmEnvironment(
    partition="infai_2",
    email="cedric.geissmann@unibas.ch")


if common_setup.is_test_run():
    ENVIRONMENT = LocalEnvironment(processes=4)

exp = Experiment()


class TranslatorDiffReport(PlanningReport):
    def get_cell(self, run):
        return ";".join(run.get(attr) for attr in self.attributes)

    def get_text(self):
        lines = []
        for runs in self.problem_runs.values():
            hashes = set([r.get("translator_output_sas_hash") for r in runs])
            if len(hashes) > 1 or None in hashes:
                lines.append(";".join([self.get_cell(r) for r in runs]))
        return "\n".join(lines)


class SameValueFilters(object):
    """Ignore runs for a task where all algorithms have the same value."""
    def __init__(self, attribute):
        self._attribute = attribute
        self._tasks_to_values = defaultdict(list)

    def _get_task(self, run):
        return (run['domain'], run['problem'])

    def store_values(self, run):
        value = run.get(self._attribute)
        self._tasks_to_values[self._get_task(run)].append(value)
        # Don't filter this run, yet.
        return True

    def filter_tasks_with_equal_values(self, run):
        values = self._tasks_to_values[self._get_task(run)]
        return len(set(values)) != 1


exp.add_fetcher(src='data/issue939-base-eval')
exp.add_fetcher(src='data/issue939-v1-eval', merge=True)

ATTRIBUTES = ["error", "run_dir", "translator_*", "translator_output_sas_hash"]
#exp.add_comparison_table_step(attributes=ATTRIBUTES)

same_value_filters = SameValueFilters("translator_output_sas_hash")
# exp.add_comparison_table_step(
#     name="filtered",
#     attributes=ATTRIBUTES,
#     filter=[same_value_filters.store_values, same_value_filters.filter_tasks_with_equal_values])

exp.add_report(TranslatorDiffReport(
        attributes=["domain", "problem", "algorithm", "run_dir"]
    ), outfile="different_output_sas.csv"
)

exp.add_report(AbsoluteReport(attributes=ATTRIBUTES))
exp.add_report(ComparativeReport([
    ('issue939-base-translate-only', 'issue939-v1-translate-only')
    ], attributes=ATTRIBUTES))

exp.run_steps()
