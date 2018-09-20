#! /usr/bin/env python
# -*- coding: utf-8 -*-

#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os
import subprocess

from lab.environments import LocalEnvironment, MaiaEnvironment
from lab.reports import Attribute, geometric_mean

from downward.reports.compare import ComparativeReport

from common_setup import IssueConfig, IssueExperiment, DEFAULT_OPTIMAL_SUITE, is_test_run

BENCHMARKS_DIR=os.path.expanduser('~/repos/downward/benchmarks')
REVISIONS = []
CONFIGS = []
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
exp.add_resource('ms_parser', 'ms-parser.py', dest='ms-parser.py')
exp.add_command('ms-parser', ['{ms_parser}'])
exp.add_suite(BENCHMARKS_DIR, SUITE)

# planner outcome attributes
perfect_heuristic = Attribute('perfect_heuristic', absolute=True, min_wins=False)

# m&s attributes
ms_construction_time = Attribute('ms_construction_time', absolute=False, min_wins=True, functions=[geometric_mean])
ms_atomic_construction_time = Attribute('ms_atomic_construction_time', absolute=False, min_wins=True, functions=[geometric_mean])
ms_abstraction_constructed = Attribute('ms_abstraction_constructed', absolute=True, min_wins=False)
ms_final_size = Attribute('ms_final_size', absolute=False, min_wins=True)
ms_out_of_memory = Attribute('ms_out_of_memory', absolute=True, min_wins=True)
ms_out_of_time = Attribute('ms_out_of_time', absolute=True, min_wins=True)
search_out_of_memory = Attribute('search_out_of_memory', absolute=True, min_wins=True)
search_out_of_time = Attribute('search_out_of_time', absolute=True, min_wins=True)

extra_attributes = [
    perfect_heuristic,

    ms_construction_time,
    ms_atomic_construction_time,
    ms_abstraction_constructed,
    ms_final_size,
    ms_out_of_memory,
    ms_out_of_time,
    search_out_of_memory,
    search_out_of_time,
]
attributes = exp.DEFAULT_TABLE_ATTRIBUTES
attributes.extend(extra_attributes)

exp.add_fetcher('data/issue707-v1-eval')
exp.add_fetcher('data/issue707-v2-pruning-variants-eval')

outfile = os.path.join(
    exp.eval_dir,
    "issue707-v1-v2-dfp-compare.html")
exp.add_report(ComparativeReport(algorithm_pairs=[
    ('%s-dfp-b50k' % 'issue707-v1', '%s-dfp-b50k-nopruneunreachable' % 'issue707-v2'),
    ('%s-dfp-b50k' % 'issue707-v1', '%s-dfp-b50k-nopruneirrelevant' % 'issue707-v2'),
    ('%s-dfp-b50k' % 'issue707-v1', '%s-dfp-b50k-noprune' % 'issue707-v2'),
    #('%s-dfp-f50k' % 'issue707-v1', '%s-dfp-f50k-nopruneunreachable' % 'issue707-v2'),
    #('%s-dfp-f50k' % 'issue707-v1', '%s-dfp-f50k-nopruneirrelevant' % 'issue707-v2'),
    #('%s-dfp-f50k' % 'issue707-v1', '%s-dfp-f50k-noprune' % 'issue707-v2'),
    #('%s-dfp-ginf' % 'issue707-v1', '%s-dfp-ginf-nopruneunreachable' % 'issue707-v2'),
    #('%s-dfp-ginf' % 'issue707-v1', '%s-dfp-ginf-nopruneirrelevant' % 'issue707-v2'),
    #('%s-dfp-ginf' % 'issue707-v1', '%s-dfp-ginf-noprune' % 'issue707-v2'),
],attributes=attributes),outfile=outfile)
exp.add_step('publish-issue707-v1-v2-dfp-compare.html', subprocess.call, ['publish', outfile])

exp.run_steps()
