#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites
from lab.reports import Attribute

import common_setup

import os


exp = common_setup.IssueExperiment(
    search_revisions=["issue479-v1"],
    configs={
        'dfp-b-50k': ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp,shrink_strategy=shrink_bisimulation(max_states=50000,threshold=1,greedy=false,group_by_h=true),label_reduction_method=all_transition_systems_with_fixpoint))'],
        'blind': ['--search', 'astar(blind())'],
    },
    suite=['airport'],
    limits={"search_time": 300},
    )

exp.add_absolute_report_step(attributes=['coverage', 'error', 'run_dir'])

exp()
