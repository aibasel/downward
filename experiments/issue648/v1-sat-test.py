#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites

from common_setup import IssueConfig, IssueExperiment


REVS = ["issue648-base", "issue648-v1"]
SUITE=suites.suite_satisficing()
SUITE.extend(suites.suite_ipc14_sat())

CONFIGS = [
    # Test lazy search with randomization
    IssueConfig("lazy_greedy_ff_randomized", [
        "--heuristic",
            "h=ff()",
        "--search",
            "lazy_greedy(h, preferred=h, randomize_successors=true)"
        ]),
    # Epsilon Greedy
    IssueConfig("lazy_epsilon_greedy_ff", [
        "--heuristic",
            "h=ff()",
        "--search",
            "lazy(epsilon_greedy(h))"
        ]),
    # Pareto
    IssueConfig("lazy_pareto_ff_cea", [
        "--heuristic",
            "h1=ff()",
        "--heuristic",
            "h2=cea()",
        "--search",
            "lazy(pareto([h1, h2]))"
        ]),
    # Type based
    IssueConfig("ff-type-const", [
        "--heuristic",
            "hff=ff(cost_type=one)",
        "--search",
            "lazy(alt([single(hff),single(hff, pref_only=true), type_based([const(1)])]),"
            "preferred=[hff],cost_type=one)"
        ]),

]

exp = IssueExperiment(
    revisions=REVS,
    configs=CONFIGS,
    suite=SUITE,
    email="silvan.sievers@unibas.ch"
)

# Absolute report commented out because a comparison table is more useful for this issue.
# (It's still in this file because someone might want to use it as a basis.)
# Scatter plots commented out for now because I have no usable matplotlib available.
# exp.add_absolute_report_step()
exp.add_comparison_table_step()
# exp.add_scatter_plot_step()

exp()
