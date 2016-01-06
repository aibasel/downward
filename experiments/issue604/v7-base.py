#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites
from lab.reports import Attribute, gm
from downward.reports.compare import CompareConfigsReport

from common_setup import IssueConfig, IssueExperiment

import os

def main(revisions=[]):
    suite = suites.suite_optimal_with_ipc11()

    configs = {
    }

    exp = IssueExperiment(
        revisions=revisions,
        configs=configs,
        suite=suite,
        test_suite=['depot:pfile1'],
        processes=4,
        email='silvan.sievers@unibas.ch',
    )
    exp.add_resource('ms_parser', 'ms-parser.py', dest='ms-parser.py')
    exp.add_command('ms-parser', ['ms_parser'])

    # planner outcome attributes
    search_out_of_memory = Attribute('search_out_of_memory', absolute=True, min_wins=True)
    search_out_of_time = Attribute('search_out_of_time', absolute=True, min_wins=True)
    perfect_heuristic = Attribute('perfect_heuristic', absolute=True, min_wins=False)
    proved_unsolvability = Attribute('proved_unsolvability', absolute=True, min_wins=False)

    # m&s attributes
    ms_construction_time = Attribute('ms_construction_time', absolute=False, min_wins=True, functions=[gm])
    ms_abstraction_constructed = Attribute('ms_abstraction_constructed', absolute=True, min_wins=False)
    ms_final_size = Attribute('ms_final_size', absolute=False, min_wins=True)
    ms_out_of_memory = Attribute('ms_out_of_memory', absolute=True, min_wins=True)
    ms_out_of_time = Attribute('ms_out_of_time', absolute=True, min_wins=True)
    ms_memory_delta = Attribute('ms_memory_delta', absolute=False, min_wins=True)

    extra_attributes = [
        search_out_of_memory,
        search_out_of_time,
        perfect_heuristic,
        proved_unsolvability,

        ms_construction_time,
        ms_abstraction_constructed,
        ms_final_size,
        ms_out_of_memory,
        ms_out_of_time,
        ms_memory_delta,
    ]
    attributes = exp.DEFAULT_TABLE_ATTRIBUTES
    attributes.extend(extra_attributes)

    exp.add_fetcher('data/issue604-v1-eval',filter_config=[
        'issue604-base-rl-b50k',
        'issue604-base-cggl-b50k',
        'issue604-base-dfp-b50k',
        'issue604-base-rl-ginf',
        'issue604-base-cggl-ginf',
        'issue604-base-dfp-ginf',
        'issue604-base-rl-f50k',
        'issue604-base-cggl-f50k',
        'issue604-base-dfp-f50k',
    ])

    exp.add_fetcher('data/issue604-v7-eval',filter_config=[
        'issue604-v7-rl-b50k',
        'issue604-v7-cggl-b50k',
        'issue604-v7-dfp-b50k',
        'issue604-v7-rl-ginf',
        'issue604-v7-cggl-ginf',
        'issue604-v7-dfp-ginf',
        'issue604-v7-rl-f50k',
        'issue604-v7-cggl-f50k',
        'issue604-v7-dfp-f50k',
    ])

    exp.add_fetcher('data/issue604-v7-rest-eval',filter_config=[
        'issue604-v7-rl-b50k',
        'issue604-v7-cggl-b50k',
        'issue604-v7-dfp-b50k',
        'issue604-v7-rl-ginf',
        'issue604-v7-cggl-ginf',
        'issue604-v7-dfp-ginf',
        'issue604-v7-rl-f50k',
        'issue604-v7-cggl-f50k',
        'issue604-v7-dfp-f50k',
    ])

    exp.add_report(CompareConfigsReport(compared_configs=[
        ('issue604-base-rl-b50k', 'issue604-v7-rl-b50k'),
        ('issue604-base-cggl-b50k', 'issue604-v7-cggl-b50k'),
        ('issue604-base-dfp-b50k', 'issue604-v7-dfp-b50k'),
        ('issue604-base-rl-ginf', 'issue604-v7-rl-ginf'),
        ('issue604-base-cggl-ginf', 'issue604-v7-cggl-ginf'),
        ('issue604-base-dfp-ginf', 'issue604-v7-dfp-ginf'),
        ('issue604-base-rl-f50k', 'issue604-v7-rl-f50k'),
        ('issue604-base-cggl-f50k', 'issue604-v7-cggl-f50k'),
        ('issue604-base-dfp-f50k', 'issue604-v7-dfp-f50k'),
    ],attributes=attributes),outfile=os.path.join(
        exp.eval_dir, 'issue604-base-v7-comparison.html'))

    exp()

main()
