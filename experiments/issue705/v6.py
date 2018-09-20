#! /usr/bin/env python
# -*- coding: utf-8 -*-

#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os

from lab.environments import LocalEnvironment, MaiaEnvironment
from lab.reports import Attribute, arithmetic_mean, geometric_mean

import common_setup
from common_setup import IssueConfig, IssueExperiment
from relativescatter import RelativeScatterPlotReport
from csv_report import CSVReport

DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue705-base", "issue705-v7", "issue705-v8"]
CONFIGS = [
    IssueConfig(
        'astar-blind',
        ['--search', 'astar(blind())'],
    )
]
SUITE = list(sorted(set(common_setup.DEFAULT_OPTIMAL_SUITE) |
                    set(common_setup.DEFAULT_SATISFICING_SUITE)))
ENVIRONMENT = MaiaEnvironment(
    priority=0, email="florian.pommerening@unibas.ch")

if common_setup.is_test_run():
    SUITE = IssueExperiment.DEFAULT_TEST_SUITE
    ENVIRONMENT = LocalEnvironment(processes=1)

exp = IssueExperiment(
    revisions=REVISIONS,
    configs=CONFIGS,
    environment=ENVIRONMENT,
)

exp.add_suite(BENCHMARKS_DIR, SUITE)
exp.add_resource('sg_parser', 'sg-parser.py', dest='sg-parser.py')
exp.add_command('sg-parser', ['{sg_parser}'])

exp.add_fetcher('data/issue705-v4-eval')

exp.add_comparison_table_step()

def add_sg_peak_mem_diff_per_task_size(run):
    mem = run.get("sg_peak_mem_diff")
    size = run.get("translator_task_size")
    if mem and size:
        run["sg_peak_mem_diff_per_task_size"] = mem / float(size)
    return run


for attr in ["total_time", "search_time", "sg_construction_time", "memory", "sg_peak_mem_diff_per_task_size"]:
    for rev1, rev2 in [("base", "v8"), ("v7", "v8")]:
        exp.add_report(RelativeScatterPlotReport(
            attributes=[attr],
            filter_algorithm=["issue705-%s-astar-blind" % rev1, "issue705-%s-astar-blind" % rev2],
            filter=add_sg_peak_mem_diff_per_task_size,
            get_category=lambda r1, r2: r1["domain"],
        ),
        outfile="issue705-%s-%s-%s.png" % (attr, rev1, rev2))

exp.run_steps()
