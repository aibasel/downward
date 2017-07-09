#! /usr/bin/env python
# -*- coding: utf-8 -*-

#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os

from lab.environments import LocalEnvironment, MaiaEnvironment

import common_setup
from common_setup import IssueConfig, IssueExperiment, get_algo_nick, get_repo_base
from relativescatter import RelativeScatterPlotReport

DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue662-base", "issue662-v1"]
CONFIGS = [
    IssueConfig(
        'astar-lmcut-static',
        ['--search', 'astar(lmcut())'],
        build_options=["release32"],
        driver_options=["--build=release32", "--search-time-limit", "60s"]
    ),
    IssueConfig(
        'astar-lmcut-dynamic',
        ['--search', 'astar(lmcut())'],
        build_options=["release32dynamic"],
        driver_options=["--build=release32dynamic", "--search-time-limit", "60s"]
    )
]
SUITE = common_setup.DEFAULT_OPTIMAL_SUITE
ENVIRONMENT = MaiaEnvironment(
    priority=0, email="florian.pommerening@unibas.ch")

if common_setup.is_test_run():
    SUITE = IssueExperiment.DEFAULT_TEST_SUITE
    ENVIRONMENT = LocalEnvironment(processes=1)

exp = IssueExperiment(
    revisions=[],
    configs=[],
    environment=ENVIRONMENT,
)

for rev in REVISIONS:
    for config in CONFIGS:
        if rev.endswith("base") and config.nick.endswith("dynamic"):
            continue
        exp.add_algorithm(
            get_algo_nick(rev, config.nick),
            get_repo_base(),
            rev,
            config.component_options,
            build_options=config.build_options,
            driver_options=config.driver_options)


exp.add_suite(BENCHMARKS_DIR, SUITE)
exp.add_absolute_report_step()
exp.add_comparison_table_step()

for attribute in ["total_time"]:
    for algo1, algo2 in [("issue662-base-astar-lmcut-static",
                          "issue662-v1-astar-lmcut-static"),
                         ("issue662-v1-astar-lmcut-static",
                          "issue662-v1-astar-lmcut-dynamic")]:
        exp.add_report(
            RelativeScatterPlotReport(
                attributes=[attribute],
                filter_algorithm=[algo1, algo2],
                get_category=lambda run1, run2: run1.get("domain"),
            ),
            outfile="{}-{}-{}-{}-{}.png".format(exp.name, attribute, config.nick, algo1, algo2)
        )

exp.run_steps()
