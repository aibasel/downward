#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites
from lab.reports import Attribute
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

exp.add_fetcher('data/issue648-v1-sat-test', parsers=['parser.py'])

# planner outcome attributes
perfect_heuristic = Attribute('perfect_heuristic', absolute=True, min_wins=False)
proved_unsolvability = Attribute('proved_unsolvability', absolute=True, min_wins=False)
out_of_memory = Attribute('out_of_memory', absolute=True, min_wins=True)
out_of_time = Attribute('out_of_time', absolute=True, min_wins=True)

extra_attributes = [
    perfect_heuristic,
    proved_unsolvability,
    out_of_memory,
    out_of_time,
]
attributes = exp.DEFAULT_TABLE_ATTRIBUTES
attributes.extend(extra_attributes)

exp.add_comparison_table_step(attributes=attributes)

exp()
