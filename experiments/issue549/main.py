#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites
from lab.reports import Attribute, gm

import common_setup


def main(revisions=None):
    SUITE = suites.suite_satisficing_with_ipc11()

    CONFIGS = {
        'cea': ['--search', 'eager_greedy(cea())'],
        'cg': ['--search', 'eager_greedy(cg())'],
        'lmcount': ['--search', 'eager_greedy(lmcount(lm_rhw()))'],
    }

    exp = common_setup.IssueExperiment(
        revisions=revisions,
        configs=CONFIGS,
        suite=SUITE,
        test_suite=['depot:pfile1'],
        processes=4,
        email='gabriele.roeger@unibas.ch',
	grid_priority=-10,
    )

    attributes = exp.DEFAULT_TABLE_ATTRIBUTES
    attributes.append('landmarks')
    attributes.append('landmarks_generation_time')
	

    exp.add_comparison_table_step(attributes=attributes)

    exp()
