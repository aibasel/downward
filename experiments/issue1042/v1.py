#! /usr/bin/env python3

import itertools
import math
import os
import subprocess

from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from lab.reports import Attribute

from downward.reports.compare import ComparativeReport

import common_setup
from common_setup import IssueConfig, IssueExperiment

DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPT_NAME = os.path.splitext(os.path.basename(__file__))[0]
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue1042-v1"]
CONFIGS = [
    IssueConfig(f'astar-lmcut', ['--search', f'astar(lmcut())'], driver_options=['--search-time-limit', '5m']),
    IssueConfig(f'astar-lmcut-sssimple', ['--search', f'astar(lmcut(),pruning=stubborn_sets_simple)'], driver_options=['--search-time-limit', '5m']),
    IssueConfig(f'astar-lmcut-sssimple-limited', ['--search', f'astar(lmcut(),pruning=limited(pruning=stubborn_sets_simple,min_required_pruning_ratio=0.2,expansions_before_checking_pruning_ratio=1000))'], driver_options=['--search-time-limit', '5m']),
    IssueConfig(f'astar-lmcut-ssatom', ['--search', f'astar(lmcut(),pruning=atom_centric_stubborn_sets)'], driver_options=['--search-time-limit', '5m']),
    IssueConfig(f'astar-lmcut-ssatom-limited', ['--search', f'astar(lmcut(),pruning=limited(pruning=atom_centric_stubborn_sets,min_required_pruning_ratio=0.2,expansions_before_checking_pruning_ratio=1000))'], driver_options=['--search-time-limit', '5m']),
]

SUITE = common_setup.DEFAULT_OPTIMAL_SUITE
ENVIRONMENT = BaselSlurmEnvironment(
    email="silvan.sievers@unibas.ch",
    partition="infai_2",
    export=[],
    # paths obtained via:
    # module purge
    # module -q load CMake/3.15.3-GCCcore-8.3.0
    # module -q load GCC/8.3.0
    # echo $PATH
    # echo $LD_LIBRARY_PATH
    setup='export PATH=/scicore/soft/apps/binutils/2.32-GCCcore-8.3.0/bin:/scicore/soft/apps/CMake/3.15.3-GCCcore-8.3.0/bin:/scicore/soft/apps/cURL/7.66.0-GCCcore-8.3.0/bin:/scicore/soft/apps/bzip2/1.0.8-GCCcore-8.3.0/bin:/scicore/soft/apps/ncurses/6.1-GCCcore-8.3.0/bin:/scicore/soft/apps/GCCcore/8.3.0/bin:/infai/sieverss/repos/bin:/infai/sieverss/local:/export/soft/lua_lmod/centos7/lmod/lmod/libexec:/usr/local/bin:/usr/bin:/usr/local/sbin:/usr/sbin:$PATH\nexport LD_LIBRARY_PATH=/scicore/soft/apps/binutils/2.32-GCCcore-8.3.0/lib:/scicore/soft/apps/cURL/7.66.0-GCCcore-8.3.0/lib:/scicore/soft/apps/bzip2/1.0.8-GCCcore-8.3.0/lib:/scicore/soft/apps/zlib/1.2.11-GCCcore-8.3.0/lib:/scicore/soft/apps/ncurses/6.1-GCCcore-8.3.0/lib:/scicore/soft/apps/GCCcore/8.3.0/lib64:/scicore/soft/apps/GCCcore/8.3.0/lib')

if common_setup.is_test_run():
    SUITE = IssueExperiment.DEFAULT_TEST_SUITE
    ENVIRONMENT = LocalEnvironment(processes=4)

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

exp.add_step('build', exp.build)
exp.add_step('start', exp.start_runs)
exp.add_fetcher(name='fetch')

attributes = list(exp.DEFAULT_TABLE_ATTRIBUTES)
attributes.append('initial_h_value')

exp.add_absolute_report_step(attributes=attributes)
exp.add_comparison_table_step(attributes=attributes)

exp.run_steps()
