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
REVISIONS = ["issue705-base", "issue705-v12"]
CONFIGS = [
    IssueConfig(
        'astar-blind',
        ['--search', 'astar(blind())'],
    ),
    IssueConfig(
        'astar-lmcut',
        ['--search', 'astar(lmcut())'],
    ),
    IssueConfig(
        'astar-cegar',
        ['--search', 'astar(cegar())'],
    ),
    IssueConfig(
        'astar-ipdb',
        ['--search', 'astar(ipdb())'],
    ),
    IssueConfig(
        'astar-lama-first',
        [],
        driver_options=['--alias', 'lama-first'],
    ),
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

exp.add_comparison_table_step()

for attr in ["total_time", "search_time", "memory"]:
    for rev1, rev2 in [("base", "v12")]:
        exp.add_report(RelativeScatterPlotReport(
            attributes=[attr],
            filter_algorithm=["issue705-%s-astar-blind" % rev1, "issue705-%s-astar-blind" % rev2],
            get_category=lambda r1, r2: r1["domain"],
        ),
        outfile="issue705-%s-%s-%s.png" % (attr, rev1, rev2))

exp.run_steps()
