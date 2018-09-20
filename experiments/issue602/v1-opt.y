#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites
from lab.reports import Attribute, gm

from common_setup import IssueConfig, IssueExperiment

SUITE_OPT14 = [
    'barman-opt14-strips',
    'cavediving-opt14-adl',
    'childsnack-opt14-strips',
    'citycar-opt14-adl',
    'floortile-opt14-strips',
    'ged-opt14-strips',
    'hiking-opt14-strips',
    'maintenance-opt14-adl',
    'openstacks-opt14-strips',
    'parking-opt14-strips',
    'tetris-opt14-strips',
    'tidybot-opt14-strips',
    'transport-opt14-strips',
    'visitall-opt14-strips',
]

def main(revisions=None):
    suite = SUITE_OPT14

    configs = [
        IssueConfig("astar_blind", [
            "--search",
            "astar(blind)"]),
        IssueConfig("astar_h2", [
            "--search",
            "astar(hm(2))"]),
        IssueConfig("astar_ipdb", [
            "--search",
            "astar(ipdb)"]),
        IssueConfig("astar_lmcount_lm_merged_rhw_hm", [
            "--search",
            "astar(lmcount(lm_merged([lm_rhw(),lm_hm(m=1)]),admissible=true),mpd=true)"]),
        IssueConfig("astar_lmcut", [
            "--search",
            "astar(lmcut)"]),
        IssueConfig("astar_hmax", [
            "--search",
            "astar(hmax)"]),
        IssueConfig("astar_merge_and_shrink_rl_fh", [
            "--search",
            "astar(merge_and_shrink("
            "merge_strategy=merge_linear(variable_order=reverse_level),"
            "shrink_strategy=shrink_fh(max_states=50000),"
            "label_reduction=exact(before_shrinking=false,"
            "before_merging=true)))"]),
        IssueConfig("astar_merge_and_shrink_dfp_bisim", [
            "--search",
            "astar(merge_and_shrink(merge_strategy=merge_dfp,"
            "shrink_strategy=shrink_bisimulation(max_states=50000,threshold=1,"
            "greedy=false),"
            "label_reduction=exact(before_shrinking=true,"
            "before_merging=false)))"]),
        IssueConfig("astar_merge_and_shrink_dfp_greedy_bisim", [
            "--search",
            "astar(merge_and_shrink(merge_strategy=merge_dfp,"
            "shrink_strategy=shrink_bisimulation(max_states=infinity,threshold=1,"
            "greedy=true),"
            "label_reduction=exact(before_shrinking=true,"
            "before_merging=false)))"]),
        IssueConfig("seq_opt_merge_and_shrink", [], driver_options=[
            "--alias", "seq-opt-merge-and-shrink"]),
        IssueConfig("seq_opt_fdss_1", [], driver_options=[
            "--alias", "seq-opt-merge-and-shrink"]),
        IssueConfig("seq_opt_fdss_2", [], driver_options=[
            "--alias", "seq-opt-fdss-2"]),
    ]

    exp = IssueExperiment(
        revisions=revisions,
        configs=configs,
        suite=suite,
        test_suite=[
            #'cavediving-opt14-adl:testing01_easy.pddl',
            #'childsnack-opt14-strips:child-snack_pfile01.pddl',
            #'citycar-opt14-adl:p2-2-2-1-2.pddl',
            #'ged-opt14-strips:d-1-2.pddl',
            'hiking-opt14-strips:ptesting-1-2-3.pddl',
            #'maintenance-opt14-adl:maintenance-1-3-010-010-2-000.pddl',
            #'tetris-opt14-strips:p01-6.pddl',
            #'transport-opt14-strips:p01.pddl',
        ],
        processes=4,
        email='silvan.sievers@unibas.ch',
    )

    exp.add_absolute_report_step()

    exp()

main(revisions=['issue602-v1'])
