#! /usr/bin/env python
# -*- coding: utf-8 -*-

import itertools
import os
import subprocess

from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from lab.reports import Attribute


import common_setup
from common_setup import IssueConfig, IssueExperiment
from relativescatter import RelativeScatterPlotReport

EXPNAME = common_setup.get_experiment_name()
DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue744-v1"]
CONFIG_DICT = {
    "eager-greedy-ff-silent": [
        "--evaluator",
        "h=ff()",
        "--search",
        "eager_greedy([h], preferred=[h], verbosity=silent)"],
    "eager-greedy-cea-silent": [
        "--evaluator",
        "h=cea()",
        "--search",
        "eager_greedy([h], preferred=[h], verbosity=silent)"],
    "lazy-greedy-add-silent": [
        "--evaluator",
        "h=add()",
        "--search",
        "lazy_greedy([h], preferred=[h], verbosity=silent)"],
    "lazy-greedy-cg-silent": [
        "--evaluator",
        "h=cg()",
        "--search",
        "lazy_greedy([h], preferred=[h], verbosity=silent)"],
    "lama-first-silent": [
        "--evaluator",
        "hlm=lmcount(lm_factory=lm_rhw(reasonable_orders=true),transform=adapt_costs(one),pref=false)",
        "--evaluator", "hff=ff(transform=adapt_costs(one))",
        "--search", """lazy_greedy([hff,hlm],preferred=[hff,hlm],
                                   cost_type=one,reopen_closed=false, verbosity=silent)"""],
    "lama-first-typed-silent": [
        "--evaluator",
        "hlm=lmcount(lm_factory=lm_rhw(reasonable_orders=true),transform=adapt_costs(one),pref=false)",
        "--evaluator", "hff=ff(transform=adapt_costs(one))",
        "--search",
            "lazy(alt([single(hff), single(hff, pref_only=true),"
            "single(hlm), single(hlm, pref_only=true), type_based([hff, g()])], boost=1000),"
            "preferred=[hff,hlm], cost_type=one, reopen_closed=false, randomize_successors=true,"
            "preferred_successors_first=false, verbosity=silent)"],

    "eager-greedy-ff-normal": [
        "--evaluator",
        "h=ff()",
        "--search",
        "eager_greedy([h], preferred=[h], verbosity=normal)"],
    "eager-greedy-cea-normal": [
        "--evaluator",
        "h=cea()",
        "--search",
        "eager_greedy([h], preferred=[h], verbosity=normal)"],
    "lazy-greedy-add-normal": [
        "--evaluator",
        "h=add()",
        "--search",
        "lazy_greedy([h], preferred=[h], verbosity=normal)"],
    "lazy-greedy-cg-normal": [
        "--evaluator",
        "h=cg()",
        "--search",
        "lazy_greedy([h], preferred=[h], verbosity=normal)"],
    "lama-first-normal": [
        "--evaluator",
        "hlm=lmcount(lm_factory=lm_rhw(reasonable_orders=true),transform=adapt_costs(one),pref=false)",
        "--evaluator", "hff=ff(transform=adapt_costs(one))",
        "--search", """lazy_greedy([hff,hlm],preferred=[hff,hlm],
                                   cost_type=one,reopen_closed=false, verbosity=normal)"""],
    "lama-first-typed-normal": [
        "--evaluator",
        "hlm=lmcount(lm_factory=lm_rhw(reasonable_orders=true),transform=adapt_costs(one),pref=false)",
        "--evaluator", "hff=ff(transform=adapt_costs(one))",
        "--search",
            "lazy(alt([single(hff), single(hff, pref_only=true),"
            "single(hlm), single(hlm, pref_only=true), type_based([hff, g()])], boost=1000),"
            "preferred=[hff,hlm], cost_type=one, reopen_closed=false, randomize_successors=true,"
            "preferred_successors_first=false, verbosity=normal)"],
}
CONFIGS = [
    IssueConfig(config_nick, config,
        driver_options=["--overall-time-limit", "30m"])
    for rev in REVISIONS
    for config_nick, config in CONFIG_DICT.items()
]
SUITE = common_setup.DEFAULT_SATISFICING_SUITE
ENVIRONMENT = BaselSlurmEnvironment(
    partition="infai_2",
    email="silvan.sievers@unibas.ch",
    export=["PATH", "DOWNWARD_BENCHMARKS"])

if common_setup.is_test_run():
    SUITE = IssueExperiment.DEFAULT_TEST_SUITE
    ENVIRONMENT = LocalEnvironment(processes=1)

exp = IssueExperiment(
    revisions=REVISIONS,
    configs=CONFIGS,
    environment=ENVIRONMENT,
)
exp.add_suite(BENCHMARKS_DIR, SUITE)

exp.add_parser(exp.EXITCODE_PARSER)
exp.add_parser(exp.TRANSLATOR_PARSER)
exp.add_parser(exp.SINGLE_SEARCH_PARSER)
exp.add_parser(exp.PLANNER_PARSER)
exp.add_parser('custom-parser.py')

exp.add_step("build", exp.build)
exp.add_step("start", exp.start_runs)
exp.add_fetcher(name="fetch")
exp.add_parse_again_step()

log_size = Attribute('log_size')
attributes = IssueExperiment.DEFAULT_TABLE_ATTRIBUTES + [log_size]

exp.add_absolute_report_step(attributes=attributes)
#exp.add_comparison_table_step()

sort_spec = [('log_size', 'desc')]
attributes = ['run_dir', 'log_size']
exp.add_sorted_report_step(attributes=attributes, sort_spec=sort_spec,filter_algorithm=[
    "{}-eager-greedy-ff-silent".format(REVISIONS[0]),
    "{}-eager-greedy-cea-silent".format(REVISIONS[0]),
    "{}-lazy-greedy-add-silent".format(REVISIONS[0]),
    "{}-lazy-greedy-cg-silent".format(REVISIONS[0]),
    "{}-lama-first-silent".format(REVISIONS[0]),
    "{}-lama-first-typed-silent".format(REVISIONS[0]),
],name="silent")
exp.add_sorted_report_step(attributes=attributes, sort_spec=sort_spec,filter_algorithm=[
    "{}-eager-greedy-ff-normal".format(REVISIONS[0]),
    "{}-eager-greedy-cea-normal".format(REVISIONS[0]),
    "{}-lazy-greedy-add-normal".format(REVISIONS[0]),
    "{}-lazy-greedy-cg-normal".format(REVISIONS[0]),
    "{}-lama-first-normal".format(REVISIONS[0]),
    "{}-lama-first-typed-normal".format(REVISIONS[0]),
],name="normal")

exp.run_steps()
