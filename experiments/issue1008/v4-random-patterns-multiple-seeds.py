#! /usr/bin/env python3

import itertools
import math
import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from lab.reports import Attribute

from downward.reports.compare import ComparativeReport

import common_setup
from common_setup import IssueConfig, IssueExperiment
from average_report import AverageAlgorithmReport

DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPT_NAME = os.path.splitext(os.path.basename(__file__))[0]
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue1008-v3", "issue1008-v4"]
MAX_TIME=20
if common_setup.is_test_run():
    MAX_TIME=2
CONFIGS = []
for random_seed in range(2018, 2028):
    ### single random pattern
    CONFIGS.append(IssueConfig(f'srnd-bidi-pdb1m-pdbs10m-t20-s{random_seed}', ['--search', f'astar(pdb(random_pattern(max_pdb_size=1000000,max_time={MAX_TIME},verbosity=normal,random_seed={random_seed},bidirectional=true)),verbosity=silent)'], driver_options=['--search-time-limit', '5m']))

    ### multiple random patterns
    CONFIGS.append(IssueConfig(f'mrnd-bidi-pdb1m-pdbs10m-t20-stag1-s{random_seed}', ['--search', f'astar(cpdbs(random_patterns(max_pdb_size=1000000,max_collection_size=10000000,pattern_generation_max_time=infinity,total_max_time={MAX_TIME},stagnation_limit=1,random_seed={random_seed},verbosity=normal,bidirectional=true)),verbosity=silent)'], driver_options=['--search-time-limit', '5m']))

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
exp.add_parser('random-pattern-parser.py')

cpdbs_num_patterns = Attribute('cpdbs_num_patterns', absolute=False, min_wins=True)
cpdbs_total_pdb_size = Attribute('cpdbs_total_pdb_size', absolute=False, min_wins=True)
cpdbs_computation_time = Attribute('cpdbs_computation_time', absolute=False, min_wins=True)
score_cpdbs_computation_time = Attribute('score_cpdbs_computation_time', absolute=True, min_wins=False)
random_pattern_num_iterations = Attribute('random_pattern_num_iterations', absolute=False, min_wins=True)
random_pattern_num_patterns = Attribute('random_pattern_num_patterns', absolute=False, min_wins=True)
random_pattern_total_pdb_size = Attribute('random_pattern_total_pdb_size', absolute=False, min_wins=True)
random_pattern_computation_time = Attribute('random_pattern_computation_time', absolute=False, min_wins=True)
score_random_pattern_computation_time = Attribute('score_random_pattern_computation_time', absolute=True, min_wins=False)

attributes=[
    'coverage', 'search_time', 'total_time',
    'expansions_until_last_jump', 'score_search_time',
    'score_total_time', 'score_memory', 'score_expansions',
    'initial_h_value', cpdbs_num_patterns, cpdbs_total_pdb_size,
    cpdbs_computation_time, score_cpdbs_computation_time,
    random_pattern_num_iterations, random_pattern_num_patterns, random_pattern_total_pdb_size,
    random_pattern_computation_time, score_random_pattern_computation_time,
]

exp.add_absolute_report_step(attributes=['coverage'])

def add_computation_time_score(run):
    """
    Convert random_pattern/cpdbs computation time into scores in the range [0, 1].

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

    run['score_random_pattern_computation_time'] = log_score(run.get('random_pattern_computation_time'), min_bound=1.0, max_bound=MAX_TIME)
    run['score_cpdbs_computation_time'] = log_score(run.get('cpdbs_computation_time'), min_bound=1.0, max_bound=MAX_TIME)
    return run

exp.add_report(
    AverageAlgorithmReport(
        algo_name_suffixes=['-s{}'.format(seed) for seed in range(2018,2028)],
        attributes=attributes,
        filter=[add_computation_time_score],
    ),
    outfile=os.path.join(exp.eval_dir, "average", "properties"),
    name="report-average"
)

exp.add_fetcher(os.path.join(exp.eval_dir, 'average'), merge=True)
exp._configs = [
    IssueConfig('srnd-bidi-pdb1m-pdbs10m-t20', []),
    IssueConfig('mrnd-bidi-pdb1m-pdbs10m-t20-stag1', []),
]
exp.add_comparison_table_step(
    attributes=attributes,
    filter=[add_computation_time_score],
)

exp.run_steps()
