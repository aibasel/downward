#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os
from downward import suites
from common_setup import IssueConfig, IssueExperiment


REVS = ["issue551-base", "issue551-v1"]
BENCHMARKS = os.path.expanduser('~/downward-benchmarks')
SUITE = suites.suite_optimal_strips()

CONFIGS = [
    IssueConfig("seq-opt-bjolp", [], driver_options=["--alias", "seq-opt-bjolp"]),
    IssueConfig("lm_hm", [
        "--landmarks", "lm=lm_hm(2)",
        "--heuristic", "hlm=lmcount(lm)",
        "--search",    "astar(hlm)"]),
    IssueConfig("lm_exhaust", [
        "--landmarks", "lm=lm_exhaust()",
        "--heuristic", "hlm=lmcount(lm)",
        "--search",    "astar(hlm)"]),
    IssueConfig("lm_rhw", [
        "--landmarks", "lm=lm_rhw()",
        "--heuristic", "hlm=lmcount(lm)",
        "--search",    "astar(hlm)"]),
    IssueConfig("lm_zg", [
        "--landmarks", "lm=lm_zg()",
        "--heuristic", "hlm=lmcount(lm)",
        "--search",    "astar(hlm)"]),
    IssueConfig("lm_hm_max", [
        "--landmarks", "lm=lm_hm(2)",
        "--heuristic", "h1=lmcount(lm,admissible=true)",
        "--heuristic", "h2=lmcount(lm,admissible=false)",
        "--search",    "astar(max([h1,h2]))"]),
]

exp = IssueExperiment(
    revisions=REVS,
    benchmarks_dir=BENCHMARKS,
    suite=SUITE,
    configs=CONFIGS,
    processes=4,
    email="manuel.heusner@unibas.ch"
)

exp.add_comparison_table_step()

exp()
