#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites

from common_setup import IssueConfig, IssueExperiment


REVS = ["issue592-base", "issue592-v3"]
SUITE = suites.suite_optimal_strips()

CONFIGS = [
    IssueConfig("lm_zg", [
        "--landmarks",
        "lm=lm_zg()",
        "--heuristic",
        "hlm=lmcount(lm)",
        "--search",
        "astar(hlm)"]),
    IssueConfig("lm_exhaust", [
        "--landmarks",
        "lm=lm_exhaust()",
        "--heuristic",
        "hlm=lmcount(lm)",
        "--search",
        "astar(hlm)"]),
    IssueConfig("lm_hm", [
        "--landmarks",
        "lm=lm_hm(2)",
        "--heuristic",
        "hlm=lmcount(lm)",
        "--search",
        "astar(hlm)"]),
    IssueConfig("lm_hm_max", [
        "--landmarks",
        "lm=lm_hm(2)",
        "--heuristic",
        "h1=lmcount(lm,admissible=true)",
        "--heuristic",
        "h2=lmcount(lm,admissible=false)",
        "--search",
        "astar(max([h1,h2]))"]),
]

exp = IssueExperiment(
    revisions=REVS,
    configs=CONFIGS,
    suite=SUITE,
    email="manuel.heusner@unibas.ch"
)

exp.add_comparison_table_step()

exp()
