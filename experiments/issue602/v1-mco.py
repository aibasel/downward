#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites
from lab.reports import Attribute, gm

from common_setup import IssueConfig, IssueExperiment

SUITE_MCO14 = [
    'barman-mco14-strips',
    'cavediving-mco14-adl',
    'childsnack-mco14-strips',
    'citycar-mco14-adl',
    'floortile-mco14-strips',
    'ged-mco14-strips',
    'hiking-mco14-strips',
    'maintenance-mco14-adl',
    'openstacks-mco14-strips',
    'parking-mco14-strips',
    'tetris-mco14-strips',
    'thoughtful-mco14-strips',
    'transport-mco14-strips',
    'visitall-mco14-strips',
]

def main(revisions=None):
    suite = SUITE_MCO14

    configs = [
        IssueConfig("astar_goalcount", [
            "--search",
            "astar(goalcount)"]),
        IssueConfig("eager_greedy_ff", [
            "--heuristic",
            "h=ff()",
            "--search",
            "eager_greedy(h, preferred=h)"]),
        IssueConfig("eager_greedy_add", [
            "--heuristic",
            "h=add()",
            "--search",
            "eager_greedy(h, preferred=h)"]),
        IssueConfig("eager_greedy_cg", [
            "--heuristic",
            "h=cg()",
            "--search",
            "eager_greedy(h, preferred=h)"]),
        IssueConfig("eager_greedy_cea", [
            "--heuristic",
            "h=cea()",
            "--search",
            "eager_greedy(h, preferred=h)"]),
        IssueConfig("lazy_greedy_ff", [
            "--heuristic",
            "h=ff()",
            "--search",
            "lazy_greedy(h, preferred=h)"]),
        IssueConfig("lazy_greedy_add", [
            "--heuristic",
            "h=add()",
            "--search",
            "lazy_greedy(h, preferred=h)"]),
        IssueConfig("lazy_greedy_cg", [
            "--heuristic",
            "h=cg()",
            "--search",
            "lazy_greedy(h, preferred=h)"]),
        IssueConfig("seq_sat_lama_2011", [], driver_options=[
            "--alias", "seq-sat-lama-2011"]),
        IssueConfig("seq_sat_fdss_1", [], driver_options=[
            "--alias", "seq-sat-fdss-1"]),
        IssueConfig("seq_sat_fdss_2", [], driver_options=[
            "--alias", "seq-sat-fdss-2"]),
    ]

    exp = IssueExperiment(
        revisions=revisions,
        configs=configs,
        suite=suite,
        test_suite=[
            #'cavediving-sat14-adl:testing01_easy.pddl',
            #'childsnack-sat14-strips:child-snack_pfile05.pddl',
            #'citycar-sat14-adl:p3-2-2-0-1.pddl',
            #'ged-sat14-strips:d-3-6.pddl',
            'hiking-sat14-strips:ptesting-1-2-7.pddl',
            #'maintenance-sat14-adl:maintenance-1-3-060-180-5-000.pddl',
            #'tetris-sat14-strips:p020.pddl',
            #'thoughtful-sat14-strips:bootstrap-typed-01.pddl',
            #'transport-sat14-strips:p01.pddl',
        ],
        processes=4,
        email='silvan.sievers@unibas.ch',
    )

    exp.add_absolute_report_step()

    exp()

main(revisions=['issue602-v1'])
