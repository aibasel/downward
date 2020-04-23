#! /usr/bin/env python
# -*- coding: utf-8 -*-

import itertools
import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment

from downward.reports.compare import ComparativeReport

import common_setup
from common_setup import IssueConfig, IssueExperiment
from relativescatter import RelativeScatterPlotReport

DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPT_NAME = os.path.splitext(os.path.basename(__file__))[0]
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue869-base"]
BUILDS = ["release32"]
CONFIG_NICKS = [
    ("translate", []),
]
CONFIGS = [
    IssueConfig(
        config_nick,
        config,
        build_options=[build],
        driver_options=["--build", build, "--translate"])
    for rev in REVISIONS
    for build in BUILDS
    for config_nick, config in CONFIG_NICKS
]

SUITE = [
    'agricola-opt18-strips', 'agricola-sat18-strips', 'airport',
    'airport-adl', 'assembly', 'barman-mco14-strips',
    'barman-opt11-strips', 'barman-opt14-strips', 'barman-sat11-strips',
    'barman-sat14-strips', 'blocks', 'caldera-opt18-adl',
    'caldera-sat18-adl', 'caldera-split-opt18-adl',
    'caldera-split-sat18-adl', 'cavediving-14-adl',
    'childsnack-opt14-strips', 'childsnack-sat14-strips',
    'citycar-opt14-adl', 'citycar-sat14-adl',
    'data-network-opt18-strips', 'data-network-sat18-strips', 'depot',
    'driverlog', 'elevators-opt08-strips', 'elevators-opt11-strips',
    'elevators-sat08-strips', 'elevators-sat11-strips',
    'flashfill-sat18-adl', 'floortile-opt11-strips',
    'floortile-opt14-strips', 'floortile-sat11-strips',
    'floortile-sat14-strips', 'freecell', 'ged-opt14-strips',
    'ged-sat14-strips', 'grid', 'gripper', 'hiking-agl14-strips',
    'hiking-opt14-strips', 'hiking-sat14-strips', 'logistics00',
    'logistics98', 'maintenance-opt14-adl', 'maintenance-sat14-adl',
    'miconic', 'miconic-fulladl', 'miconic-simpleadl', 'movie',
    'mprime', 'mystery', 'no-mprime', 'no-mystery',
    'nomystery-opt11-strips', 'nomystery-sat11-strips',
    'nurikabe-opt18-adl', 'nurikabe-sat18-adl', 'openstacks',
    'openstacks-agl14-strips', 'openstacks-opt08-adl',
    'openstacks-opt08-strips', 'openstacks-opt11-strips',
    'openstacks-opt14-strips', 'openstacks-sat08-adl',
    'openstacks-sat08-strips', 'openstacks-sat11-strips',
    'openstacks-sat14-strips', 'openstacks-strips',
    'optical-telegraphs', 'organic-synthesis-opt18-strips',
    'organic-synthesis-sat18-strips',
    'organic-synthesis-split-opt18-strips',
    'organic-synthesis-split-sat18-strips', 'parcprinter-08-strips',
    'parcprinter-opt11-strips', 'parcprinter-sat11-strips',
    'parking-opt11-strips', 'parking-opt14-strips',
    'parking-sat11-strips', 'parking-sat14-strips', 'pathways',
    'pathways-noneg', 'pegsol-08-strips', 'pegsol-opt11-strips',
    'pegsol-sat11-strips', 'petri-net-alignment-opt18-strips',
    'philosophers', 'pipesworld-notankage', 'pipesworld-tankage',
    'psr-large', 'psr-middle', 'psr-small', 'rovers', 'satellite',
    'scanalyzer-08-strips', 'scanalyzer-opt11-strips',
    'scanalyzer-sat11-strips', 'schedule', 'settlers-opt18-adl',
    'settlers-sat18-adl', 'snake-opt18-strips', 'snake-sat18-strips',
    'sokoban-opt08-strips', 'sokoban-opt11-strips',
    'sokoban-sat08-strips', 'sokoban-sat11-strips',
    'spider-opt18-strips', 'spider-sat18-strips', 'storage',
    'termes-opt18-strips', 'termes-sat18-strips', 'tetris-opt14-strips',
    'tetris-sat14-strips', 'thoughtful-mco14-strips',
    'thoughtful-sat14-strips', 'tidybot-opt11-strips',
    'tidybot-opt14-strips', 'tidybot-sat11-strips', 'tpp',
    'transport-opt08-strips', 'transport-opt11-strips',
    'transport-opt14-strips', 'transport-sat08-strips',
    'transport-sat11-strips', 'transport-sat14-strips', 'trucks',
    'trucks-strips', 'visitall-opt11-strips', 'visitall-opt14-strips',
    'visitall-sat11-strips', 'visitall-sat14-strips',
    'woodworking-opt08-strips', 'woodworking-opt11-strips',
    'woodworking-sat08-strips', 'woodworking-sat11-strips',
    'zenotravel']
ENVIRONMENT = BaselSlurmEnvironment(
    partition="infai_2",
    email="jendrik.seipp@unibas.ch",
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
#exp.add_parser(exp.SINGLE_SEARCH_PARSER)
#exp.add_parser(exp.PLANNER_PARSER)

exp.add_step('build', exp.build)
exp.add_step('start', exp.start_runs)
exp.add_fetcher(name='fetch')

exp.add_absolute_report_step(attributes=["translator_time_done", "translator_peak_memory"])
#exp.add_comparison_table_step()

exp.run_steps()
