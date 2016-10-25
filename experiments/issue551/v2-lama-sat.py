#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os
from downward import suites
from common_setup import IssueConfig, IssueExperiment


REVS = ["issue551-base", "issue551-v2"]
BENCHMARKS = os.path.expanduser('~/downward-benchmarks')
SUITE = suites.suite_satisficing_strips()

CONFIGS = [
    IssueConfig("lama-first", [], driver_options=["--alias", "lama-first"]),
    IssueConfig("lm_hm", [
        "--landmarks", "lm=lm_hm(2)",
        "--heuristic", "hlm=lmcount(lm)",
        "--search",    "lazy_greedy(hlm)"]),
    IssueConfig("lm_exhaust", [
        "--landmarks", "lm=lm_exhaust()",
        "--heuristic", "hlm=lmcount(lm)",
        "--search",    "lazy_greedy(hlm)"]),
    IssueConfig("lm_rhw", [
        "--landmarks", "lm=lm_rhw()",
        "--heuristic", "hlm=lmcount(lm)",
        "--search",    "lazy_greedy(hlm)"]),
    IssueConfig("lm_zg", [
        "--landmarks", "lm=lm_zg()",
        "--heuristic", "hlm=lmcount(lm)",
        "--search",    "lazy_greedy(hlm)"]),
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
