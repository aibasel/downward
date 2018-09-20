#! /usr/bin/env python
# -*- coding: utf-8 -*-

#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os

from lab.environments import LocalEnvironment, MaiaEnvironment
from lab.reports import Attribute, geometric_mean

from common_setup import IssueConfig, IssueExperiment, DEFAULT_OPTIMAL_SUITE, is_test_run

BENCHMARKS_DIR=os.path.expanduser('~/repos/downward/benchmarks')
REVISIONS = ["issue710-base", "issue710-v1"]
CONFIGS = [
    IssueConfig('cpdbs-hc', ['--search', 'astar(cpdbs(patterns=hillclimbing()))']),
    IssueConfig('cpdbs-hc900', ['--search', 'astar(cpdbs(patterns=hillclimbing(max_time=900)))']),
]
SUITE = DEFAULT_OPTIMAL_SUITE
ENVIRONMENT = MaiaEnvironment(
    priority=0, email='silvan.sievers@unibas.ch')

if is_test_run():
    SUITE = ['depot:p01.pddl', 'depot:p02.pddl', 'parcprinter-opt11-strips:p01.pddl', 'parcprinter-opt11-strips:p02.pddl', 'mystery:prob07.pddl']
    ENVIRONMENT = LocalEnvironment(processes=4)

exp = IssueExperiment(
    revisions=REVISIONS,
    configs=CONFIGS,
    environment=ENVIRONMENT,
)
exp.add_resource('ipdb_parser', 'ipdb-parser.py', dest='ipdb-parser.py')
exp.add_command('ipdb-parser', ['{ipdb_parser}'])
exp.add_suite(BENCHMARKS_DIR, SUITE)

# ipdb attributes
extra_attributes = [
    Attribute('hc_iterations', absolute=True, min_wins=True),
    Attribute('hc_num_patters', absolute=True, min_wins=True),
    Attribute('hc_size', absolute=True, min_wins=True),
    Attribute('hc_num_generated', absolute=True, min_wins=True),
    Attribute('hc_num_rejected', absolute=True, min_wins=True),
    Attribute('hc_max_pdb_size', absolute=True, min_wins=True),
    Attribute('hc_hill_climbing_time', absolute=False, min_wins=True, functions=[geometric_mean]),
    Attribute('hc_total_time', absolute=False, min_wins=True, functions=[geometric_mean]),
    Attribute('cpdbs_time', absolute=False, min_wins=True, functions=[geometric_mean]),
]
attributes = exp.DEFAULT_TABLE_ATTRIBUTES
attributes.extend(extra_attributes)

exp.add_comparison_table_step(attributes=attributes)
exp.add_scatter_plot_step()

exp.run_steps()
