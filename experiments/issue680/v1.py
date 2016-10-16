#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os
import suites
from lab.reports import Attribute, gm

from common_setup import IssueConfig, IssueExperiment
from relativescatter import RelativeScatterPlotReport

def main(revisions=None):
    benchmarks_dir=os.path.expanduser('~/projects/downward/benchmarks')
    suite=suites.suite_optimal()

    configs = []

    for osi in ['103', '107']:
        configs += [
            IssueConfig(
                'astar_seq_landmarks_OSI%s' % osi,
                ['--search', 'astar(operatorcounting([state_equation_constraints(), lmcut_constraints()]))'],
                build_options=['issue680_OSI%s' % osi],
                driver_options=['--build=issue680_OSI%s' % osi]
            ),
            IssueConfig(
                'astar_diverse_potentials_OSI%s' % osi,
                ['--search', 'astar(diverse_potentials())'],
                build_options=['issue680_OSI%s' % osi],
                driver_options=['--build=issue680_OSI%s' % osi]
            ),
            IssueConfig(
                'astar_lmcount_OSI%s' % osi,
                ['--search', 'astar(lmcount(lm_merged([lm_rhw(),lm_hm(m=1)]),admissible=true,optimal=true),mpd=true)'],
                build_options=['issue680_OSI%s' % osi],
                driver_options=['--build=issue680_OSI%s' % osi]
            ),
        ]

    exp = IssueExperiment(
        benchmarks_dir=benchmarks_dir,
        suite=suite,
        revisions=revisions,
        configs=configs,
        test_suite=['depot:p01.pddl', 'gripper:prob01.pddl'],
        processes=4,
        email='florian.pommerening@unibas.ch',
    )

    attributes = exp.DEFAULT_TABLE_ATTRIBUTES

    exp.add_comparison_table_step()

    for attribute in ["memory", "total_time"]:
        for config in ['astar_seq_landmarks', 'astar_diverse_potentials', 'astar_lmcount']:
            exp.add_report(
                RelativeScatterPlotReport(
                    attributes=[attribute],
                    filter_config=["{}-{}_OSI{}".format(revisions[0], config, osi) for osi in ['103', '107']],
                    get_category=lambda run1, run2: run1.get("domain"),
                ),
                outfile="{}-{}-{}.png".format(exp.name, attribute, config)
            )

    exp()

main(revisions=['issue680-v1'])
