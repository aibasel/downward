#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites

from common_setup import IssueConfig, IssueExperiment


REVS = ["issue481-base", "issue481-v1"]
SUITE = suites.suite_satisficing_with_ipc11()

CONFIGS = [
    # Greedy (tests single and alternating open lists)
    IssueConfig("eager_greedy_ff", [
        "--heuristic",
            "h=ff()",
        "--search",
            "eager_greedy(h, preferred=h)"
        ]),
    IssueConfig("lazy_greedy_ff", [
        "--heuristic",
            "h=ff()",
        "--search",
            "lazy_greedy(h, preferred=h)"
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
    # Single Buckets
    IssueConfig("lazy_single_buckets_ff", [
        "--heuristic",
            "h=ff()",
        "--search",
            "lazy(single_buckets(h))"
        ]),
    # Type based (from issue455)
    IssueConfig("ff-type-const", [
        "--heuristic",
            "hff=ff(cost_type=one)",
        "--search",
            "lazy(alt([single(hff),single(hff, pref_only=true), type_based([const(1)])]),"
            "preferred=[hff],cost_type=one)"
        ]),
    IssueConfig("lama-first", [
        "--heuristic",
            "hlm,hff=lm_ff_syn(lm_rhw(reasonable_orders=true,lm_cost_type=one,cost_type=one))",
        "--search",
            "lazy(alt([single(hff),single(hff, pref_only=true), single(hlm), single(hlm, pref_only=true)]),"
            "preferred=[hff,hlm],cost_type=one)"
        ]),
    IssueConfig("lama-first-types-ff-g", [
        "--heuristic",
            "hlm,hff=lm_ff_syn(lm_rhw(reasonable_orders=true,lm_cost_type=one,cost_type=one))",
        "--search",
            "lazy(alt([single(hff),single(hff, pref_only=true), single(hlm), single(hlm, pref_only=true), type_based([hff, g()])]),"
            "preferred=[hff,hlm],cost_type=one)"
        ]),

]

exp = IssueExperiment(
    revisions=REVS,
    configs=CONFIGS,
    suite=SUITE,
    email="malte.helmert@unibas.ch"
)

# Absolute report commented out because a comparison table is more useful for this issue.
# (It's still in this file because someone might want to use it as a basis.)
# Scatter plots commented out for now because I have no usable matplotlib available.
# exp.add_absolute_report_step()
exp.add_comparison_table_step()
# exp.add_scatter_plot_step()

exp()
