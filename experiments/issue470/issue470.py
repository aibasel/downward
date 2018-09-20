#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites

import common_setup

CONFIGS = {
        'astar_merge_and_shrink_bisim': [
            '--search',
            'astar(merge_and_shrink('
            + 'merge_strategy=merge_linear(variable_order=reverse_level),'
            + 'shrink_strategy=shrink_bisimulation(max_states=200000,greedy=false,'
            + 'group_by_h=true)))'],
        'astar_merge_and_shrink_greedy_bisim': [
            '--search',
            'astar(merge_and_shrink('
            + 'merge_strategy=merge_linear(variable_order=reverse_level),'
            + 'shrink_strategy=shrink_bisimulation(max_states=infinity,threshold=1,'
            + 'greedy=true,group_by_h=false)))'],
        'astar_merge_and_shrink_dfp_bisim': [
            '--search',
            'astar(merge_and_shrink(merge_strategy=merge_dfp,'
            + 'shrink_strategy=shrink_bisimulation(max_states=50000,threshold=1,'
            + 'greedy=false,group_by_h=true)))'],
        'astar_ipdb': [
            '--search',
            'astar(ipdb())'],
        'astar_pdb': [
            '--search',
            'astar(pdb())'],
        'astar_gapdb': [
            '--search',
            'astar(gapdb())'],
}

exp = common_setup.IssueExperiment(
    search_revisions=["issue470-base", "issue470-v1"],
    configs=CONFIGS,
    suite=suites.suite_optimal_with_ipc11(),
    )

exp.add_comparison_table_step()

exp()
