#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites
import common_setup


REVS = ["issue527-v1"]
SUITE = suites.suite_optimal_with_ipc11()

CONFIGS = {
    "astar_occ_lmcut": [
        "--search",
        "astar(operatorcounting([lmcut_constraints()]))"],
    "astar_occ_seq": [
        "--search",
        "astar(operatorcounting([state_equation_constraints()]))"],
    "astar_occ_pho_1": [
        "--search",
        "astar(operatorcounting([pho_constraints_systematic(pattern_max_size=1, only_interesting_patterns=true)]))"],
    "astar_occ_pho_2": [
        "--search",
        "astar(operatorcounting([pho_constraints_systematic(pattern_max_size=2, only_interesting_patterns=true)]))"],
    "astar_occ_pho_2_naive": [
        "--search",
        "astar(operatorcounting([pho_constraints_systematic(pattern_max_size=2, only_interesting_patterns=false)]))"],
    "astar_occ_pho_ipdb": [
        "--search",
        "astar(operatorcounting([pho_constraints_ipdb()]))"],
    "astar_cpdbs_1": [
        "--search",
        "astar(cpdbs_systematic(pattern_max_size=1, only_interesting_patterns=true))"],
    "astar_cpdbs_2": [
        "--search",
        "astar(cpdbs_systematic(pattern_max_size=2, only_interesting_patterns=true))"],
    "astar_occ_pho_2_naive": [
        "--search",
        "astar(cpdbs_systematic(pattern_max_size=2, only_interesting_patterns=false))"],
}

exp = common_setup.IssueExperiment(
    search_revisions=REVS,
    configs=CONFIGS,
    suite=SUITE,
    )

exp.add_absolute_report_step()

exp()
