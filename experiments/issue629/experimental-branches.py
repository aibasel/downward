#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os
import suites
from lab.reports import Attribute, gm

from common_setup import IssueConfig, IssueExperiment

def main(revisions=None):
    benchmarks_dir=os.path.expanduser('~/projects/downward/benchmarks')
    suite=[
        'airport',
        'depot',
        'driverlog',
        'elevators-opt08-strips',
        'elevators-opt11-strips',
        'freecell',
        'hiking-opt14-strips',
        'pipesworld-tankage',
    ]

    configs = {
        IssueConfig(
            'astar-blind-ssec',
            ['--search', 'astar(blind(), pruning=stubborn_sets_ec())']
        ),
    }

    exp = IssueExperiment(
        benchmarks_dir=benchmarks_dir,
        suite=suite,
        revisions=revisions,
        configs=configs,
        test_suite=['depot:p01.pddl'],
        processes=4,
        email='florian.pommerening@unibas.ch',
    )

    exp.add_comparison_table_step()

    exp()

# issue629-experimental-base is based on issue629-v2-base and only removed the ordering  of actions after pruning
# issue629-experimental is based on issue629-v4 and only removed the ordering  of actions after pruning
# Both branches will not be merged.
main(revisions=['issue629-experimental-base', 'issue629-experimental'])
