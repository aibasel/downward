#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites

import common_setup


CONFIGS = {
    "blind": [
        "--search",
        "astar(blind)"],
    "ff": [
        "--heuristic",
        "h=ff()",
        "--search",
        "eager_greedy(h, preferred=h)"],
}

REVS = ["issue533-base", "issue533-v1", "issue533-v1-debug"]

LIMITS = {"search_time": 300}

# We define a suite that consists of (really) all domains because for
# translator issues like this one, it's interesting what we do in
# obscure cases like airport-adl. The following is simply a list of
# all domains that were in the benchmarks directory at the time of
# this writing.
SUITE = [
    "airport",
    "airport-adl",
    "assembly",
    "barman-opt11-strips",
    "barman-sat11-strips",
    "blocks",
    "depot",
    "driverlog",
    "elevators-opt08-strips",
    "elevators-opt11-strips",
    "elevators-sat08-strips",
    "elevators-sat11-strips",
    "floortile-opt11-strips",
    "floortile-sat11-strips",
    "freecell",
    "grid",
    "gripper",
    "logistics00",
    "logistics98",
    "miconic",
    "miconic-fulladl",
    "miconic-simpleadl",
    "movie",
    "mprime",
    "mystery",
    "no-mprime",
    "no-mystery",
    "nomystery-opt11-strips",
    "nomystery-sat11-strips",
    "openstacks",
    "openstacks-opt08-adl",
    "openstacks-opt08-strips",
    "openstacks-opt11-strips",
    "openstacks-sat08-adl",
    "openstacks-sat08-strips",
    "openstacks-sat11-strips",
    "openstacks-strips",
    "optical-telegraphs",
    "parcprinter-08-strips",
    "parcprinter-opt11-strips",
    "parcprinter-sat11-strips",
    "parking-opt11-strips",
    "parking-sat11-strips",
    "pathways",
    "pathways-noneg",
    "pegsol-08-strips",
    "pegsol-opt11-strips",
    "pegsol-sat11-strips",
    "philosophers",
    "pipesworld-notankage",
    "pipesworld-tankage",
    "psr-large",
    "psr-middle",
    "psr-small",
    "rovers",
    "satellite",
    "scanalyzer-08-strips",
    "scanalyzer-opt11-strips",
    "scanalyzer-sat11-strips",
    "schedule",
    "sokoban-opt08-strips",
    "sokoban-opt11-strips",
    "sokoban-sat08-strips",
    "sokoban-sat11-strips",
    "storage",
    "tidybot-opt11-strips",
    "tidybot-sat11-strips",
    "tpp",
    "transport-opt08-strips",
    "transport-opt11-strips",
    "transport-sat08-strips",
    "transport-sat11-strips",
    "trucks",
    "trucks-strips",
    "visitall-opt11-strips",
    "visitall-sat11-strips",
    "woodworking-opt08-strips",
    "woodworking-opt11-strips",
    "woodworking-sat08-strips",
    "woodworking-sat11-strips",
    "zenotravel",
]

exp = common_setup.IssueExperiment(
    revisions=REVS,
    configs=CONFIGS,
    suite=SUITE,
    limits=LIMITS,
    )

exp.add_comparison_table_step(
    attributes=exp.DEFAULT_TABLE_ATTRIBUTES +
    ["translate_*", "translator_*"])

exp()
