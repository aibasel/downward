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
REVISIONS = ["issue1007-base-v2", "issue1007-v16", "issue1007-v17"]
random_seed=2018
MAX_TIME=900
if common_setup.is_test_run():
    MAX_TIME=2
CONFIGS = [
    IssueConfig(f'cpdbs-hillclimbing-pdb1m-pdbs10m-t100-s{random_seed}', ['--search', f'astar(cpdbs(hillclimbing(max_time={MAX_TIME},pdb_max_size=1000000,collection_max_size=10000000,random_seed={random_seed})),verbosity=silent)']),
    IssueConfig('cpdbs-sys2', ['--search', 'astar(cpdbs(systematic(pattern_max_size=2)),verbosity=silent)']),
    IssueConfig('cpdbs-sys3', ['--search', 'astar(cpdbs(systematic(pattern_max_size=3)),verbosity=silent)']),

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
exp.add_parser('cpdbs-parser.py')

cpdbs_num_patterns = Attribute('cpdbs_num_patterns', absolute=False, min_wins=True)
cpdbs_total_pdb_size = Attribute('cpdbs_total_pdb_size', absolute=False, min_wins=True)
cpdbs_computation_time = Attribute('cpdbs_computation_time', absolute=False, min_wins=True)
score_cpdbs_computation_time = Attribute('score_cpdbs_computation_time', absolute=True, min_wins=False)

attributes = [
    cpdbs_num_patterns,
    cpdbs_total_pdb_size,
    cpdbs_computation_time,
    score_cpdbs_computation_time,
]
attributes.extend(exp.DEFAULT_TABLE_ATTRIBUTES)
attributes.append('initial_h_value')

def add_computation_time_score(run):
    """
    Convert cegar/cpdbs computation time into scores in the range [0, 1].

    Best possible performance in a task is counted as 1, while failure
    to construct the heuristic and worst performance are counted as 0.

    """
    def log_score(value, min_bound, max_bound):
        assert min_bound < max_bound
        if value is None:
            return 0
        value = max(value, min_bound)
        value = min(value, max_bound)
        raw_score = math.log(value) - math.log(max_bound)
        best_raw_score = math.log(min_bound) - math.log(max_bound)
        return raw_score / best_raw_score

    run['score_cpdbs_computation_time'] = log_score(run.get('cpdbs_computation_time'), min_bound=1.0, max_bound=MAX_TIME)
    return run

exp.add_absolute_report_step(attributes=attributes,filter=[add_computation_time_score])

exp.add_fetcher('data/issue1007-v15-ipdb-sys-eval', filter_algorithm=[
    f'issue1007-v15-cpdbs-hillclimbing-pdb1m-pdbs10m-t100-s{random_seed}',
    f'issue1007-v15-cpdbs-sys2',
    f'issue1007-v15-cpdbs-sys3',
],merge=True)

exp.add_comparison_table_step_for_revision_pairs(
    revision_pairs=[
        ("issue1007-v15", "issue1007-v16"),
        ("issue1007-v16", "issue1007-v17"),
        ("issue1007-v15", "issue1007-v17"),
        ("issue1007-base-v2", "issue1007-v16"),
        ("issue1007-base-v2", "issue1007-v17"),
    ],
    attributes=attributes,
    filter=[add_computation_time_score],
)

exp.run_steps()
