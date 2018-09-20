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
        for cplex in ['1251', '1263']:
            if osi == '107' and cplex == '1251':
                # incompatible versions
                continue
            configs += [
                IssueConfig(
                    'astar_initial_state_potential_OSI%s_CPLEX%s' % (osi, cplex),
                    ['--search', 'astar(initial_state_potential())'],
                    build_options=['issue680_OSI%s_CPLEX%s' % (osi, cplex)],
                    driver_options=['--build=issue680_OSI%s_CPLEX%s' % (osi, cplex)]
                ),
                IssueConfig(
                    'astar_sample_based_potentials_OSI%s_CPLEX%s' % (osi, cplex),
                    ['--search', 'astar(sample_based_potentials())'],
                    build_options=['issue680_OSI%s_CPLEX%s' % (osi, cplex)],
                    driver_options=['--build=issue680_OSI%s_CPLEX%s' % (osi, cplex)]
                ),
                IssueConfig(
                    'astar_all_states_potential_OSI%s_CPLEX%s' % (osi, cplex),
                    ['--search', 'astar(all_states_potential())'],
                    build_options=['issue680_OSI%s_CPLEX%s' % (osi, cplex)],
                    driver_options=['--build=issue680_OSI%s_CPLEX%s' % (osi, cplex)]
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

    domains = suites.suite_optimal_strips()

    exp.add_absolute_report_step(filter_domain=domains)

    for attribute in ["memory", "total_time"]:
        for config in ['astar_initial_state_potential', 'astar_sample_based_potentials', 'astar_all_states_potential']:
            exp.add_report(
                RelativeScatterPlotReport(
                    attributes=[attribute],
                    filter_config=["{}-{}_OSI{}_CPLEX1263".format(revisions[0], config, osi) for osi in ['103', '107']],
                    filter_domain=domains,
                    get_category=lambda run1, run2: run1.get("domain"),
                ),
                outfile="{}-{}-{}_CPLEX1263.png".format(exp.name, attribute, config)
            )
            exp.add_report(
                RelativeScatterPlotReport(
                    attributes=[attribute],
                    filter_config=["{}-{}_OSI103_CPLEX{}".format(revisions[0], config, cplex) for cplex in ['1251', '1263']],
                    filter_domain=domains,
                    get_category=lambda run1, run2: run1.get("domain"),
                ),
                outfile="{}-{}-{}_OSI103.png".format(exp.name, attribute, config)
            )

    exp()

main(revisions=['issue680-v2'])
