#! /usr/bin/env python3

import itertools
import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from lab.reports import Attribute, geometric_mean

from downward.reports.compare import ComparativeReport

import common_setup
from common_setup import IssueConfig, IssueExperiment

ARCHIVE_PATH = "ai/downward/issue927"
DIR = os.path.dirname(os.path.abspath(__file__))
REPO_DIR = os.environ["DOWNWARD_REPO"]
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue1019-base", "issue1019-v1"]
BUILDS = ["release"]
MAX_TIME=2
if common_setup.is_running_on_cluster():
    MAX_TIME=100
CONFIG_NICKS = [
    ('cpdbs-cegar', ['--search', f'astar(cpdbs(multiple_cegar(max_pdb_size=1000000,max_collection_size=10000000,pattern_generation_max_time=10,total_max_time={MAX_TIME},stagnation_limit=20,blacklist_trigger_percentage=0.75,enable_blacklist_on_stagnation=true,use_wildcard_plans=true)),verbosity=silent)']),
    ('cpdbs-hc', ['--search', f'astar(cpdbs(hillclimbing(pdb_max_size=1000000,collection_max_size=10000000,max_time={MAX_TIME})),verbosity=silent)']),
    ('cpdbs-sys', ['--search', 'astar(cpdbs(systematic(2)),verbosity=silent)']),
]
CONFIGS = [
    IssueConfig(
        config_nick,
        config,
        build_options=[build],
        driver_options=["--build", build, "--search-time-limit", "5m"])
    for build in BUILDS
    for config_nick, config in CONFIG_NICKS
]

SUITE = common_setup.DEFAULT_OPTIMAL_SUITE
ENVIRONMENT = BaselSlurmEnvironment(
    partition="infai_2",
    email="silvan.sievers@unibas.ch",
    # paths obtained via:
    # module purge
    # module -q CMake/3.23.1-GCCcore-11.3.0
    # echo $PATH
    # echo $LD_LIBRARY_PATH
    setup='export PATH=/scicore/soft/apps/CMake/3.23.1-GCCcore-11.3.0/bin:/scicore/soft/apps/libarchive/3.6.1-GCCcore-11.3.0/bin:/scicore/soft/apps/XZ/5.2.5-GCCcore-11.3.0/bin:/scicore/soft/apps/cURL/7.83.0-GCCcore-11.3.0/bin:/scicore/soft/apps/OpenSSL/1.1/bin:/scicore/soft/apps/bzip2/1.0.8-GCCcore-11.3.0/bin:/scicore/soft/apps/ncurses/6.3-GCCcore-11.3.0/bin:/scicore/soft/apps/GCCcore/11.3.0/bin:/infai/sieverss/repos/downward/dev/experiments/issue1019/.venv/bin:/scicore/soft/apps/Python/3.9.5-GCCcore-10.3.0/bin:/scicore/soft/apps/XZ/5.2.5-GCCcore-10.3.0/bin:/scicore/soft/apps/SQLite/3.35.4-GCCcore-10.3.0/bin:/scicore/soft/apps/Tcl/8.6.11-GCCcore-10.3.0/bin:/scicore/soft/apps/ncurses/6.2-GCCcore-10.3.0/bin:/scicore/soft/apps/bzip2/1.0.8-GCCcore-10.3.0/bin:/scicore/soft/apps/binutils/2.36.1-GCCcore-10.3.0/bin:/scicore/soft/apps/GCCcore/10.3.0/bin:/infai/sieverss/repos/bin:/infai/sieverss/local:/export/soft/lua_lmod/centos7/lmod/lmod/libexec:/usr/local/bin:/usr/bin:/usr/local/sbin:/usr/sbin:$PATH\nexport LD_LIBRARY_PATH=/scicore/soft/apps/libarchive/3.6.1-GCCcore-11.3.0/lib:/scicore/soft/apps/XZ/5.2.5-GCCcore-11.3.0/lib:/scicore/soft/apps/cURL/7.83.0-GCCcore-11.3.0/lib:/scicore/soft/apps/OpenSSL/1.1/lib:/scicore/soft/apps/bzip2/1.0.8-GCCcore-11.3.0/lib:/scicore/soft/apps/zlib/1.2.12-GCCcore-11.3.0/lib:/scicore/soft/apps/ncurses/6.3-GCCcore-11.3.0/lib:/scicore/soft/apps/GCCcore/11.3.0/lib64')

if common_setup.is_test_run():
    SUITE = IssueExperiment.DEFAULT_TEST_SUITE
    ENVIRONMENT = LocalEnvironment(processes=4)

exp = IssueExperiment(
    REPO_DIR,
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

attributes = exp.DEFAULT_TABLE_ATTRIBUTES

exp.add_comparison_table_step(attributes=attributes)

exp.add_archive_step(ARCHIVE_PATH)

exp.run_steps()
