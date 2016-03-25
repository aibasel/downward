#! /usr/bin/env python
# -*- coding: utf-8 -*-

from common_setup import IssueConfig, IssueExperiment
import suites


heuristics = [
    "{}(cache_estimates=false)".format(h) for h in (
        "pdb", "cpdbs", "diverse_potentials", "all_states_potential",
        "initial_state_potential", "sample_based_potentials")]

max_eval = "max([{}])".format(",".join(heuristics))
ipc_max = "ipc_max([{}],cache_estimates=false)".format(",".join(heuristics))

configs = [
    IssueConfig(
        name,
        ["--search", "astar({})".format(eval_)])
    for name, eval_ in [("max", max_eval), ("ipc_max", ipc_max)]
]
revisions = ["1e84d77e4e37"]

exp = IssueExperiment(
    revisions=revisions,
    configs=configs,
    suite=suites.suite_optimal_strips(),
    test_suite=["depot:pfile1"],
    email="jendrik.seipp@unibas.ch",
)

exp.add_absolute_report_step()

exp()
