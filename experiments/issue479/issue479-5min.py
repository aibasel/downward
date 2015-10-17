#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites
from lab.reports import Attribute

import common_setup

import os


exp = common_setup.IssueExperiment(
    search_revisions=["issue479-v2"],
    configs={
        'dfp-b-50k': ['--search', 'astar(merge_and_shrink(shrink_strategy=shrink_bisimulation(max_states=100000,threshold=1,greedy=false),merge_strategy=merge_dfp(),label_reduction=label_reduction(before_shrinking=true, before_merging=false)))'],
        'blind': ['--search', 'astar(blind())'],
    },
    suite=['airport'],
    limits={"search_time": 300},
    )

exp.add_absolute_report_step(attributes=['coverage', 'error', 'run_dir'])

exp()
