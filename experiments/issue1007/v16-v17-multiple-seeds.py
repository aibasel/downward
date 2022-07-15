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
REVISIONS = ["issue1007-v16", "issue1007-v17"]
MAX_TIME=20
if common_setup.is_test_run():
    MAX_TIME=1
CONFIGS = []
for random_seed in range(2018, 2028):
    ### single cegar
    CONFIGS.append(IssueConfig(f'cpdbs-singlecegar-regularplans-pdb1m-pdbs10m-t20-s{random_seed}', ['--search', f'astar(cpdbs(single_cegar(use_wildcard_plans=false,max_time={MAX_TIME},max_pdb_size=1000000,max_collection_size=10000000,random_seed={random_seed},verbosity=normal)),verbosity=silent)'], driver_options=['--search-time-limit', '5m']))

    ### multiple cegar
    CONFIGS.append(IssueConfig(f'cpdbs-multiplecegar-wildcardplans-pdb1m-pdbs10m-t20-blacklist0.75-stag4-s{random_seed}', ['--search', f'astar(cpdbs(multiple_cegar(use_wildcard_plans=true,max_time=100,max_pdb_size=1000000,max_collection_size=10000000,random_seed={random_seed},total_max_time={MAX_TIME},stagnation_limit=4,blacklist_trigger_percentage=0.75,enable_blacklist_on_stagnation=true,verbosity=normal)),verbosity=silent)'], driver_options=['--search-time-limit', '5m']))

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
exp.add_parser('cegar-parser.py')

cpdbs_num_patterns = Attribute('cpdbs_num_patterns', absolute=False, min_wins=True)
cpdbs_total_pdb_size = Attribute('cpdbs_total_pdb_size', absolute=False, min_wins=True)
cpdbs_computation_time = Attribute('cpdbs_computation_time', absolute=False, min_wins=True)
score_cpdbs_computation_time = Attribute('score_cpdbs_computation_time', absolute=True, min_wins=False)
cegar_num_iterations = Attribute('cegar_num_iterations', absolute=False, min_wins=True)
cegar_num_patterns = Attribute('cegar_num_patterns', absolute=False, min_wins=True)
cegar_total_pdb_size = Attribute('cegar_total_pdb_size', absolute=False, min_wins=True)
cegar_computation_time = Attribute('cegar_computation_time', absolute=False, min_wins=True)
score_cegar_computation_time = Attribute('score_cegar_computation_time', absolute=True, min_wins=False)

exp.add_absolute_report_step(attributes=['coverage'])

### compare against v15
exp.add_fetcher('data/issue1007-v15-multiple-seeds-eval',merge=True,filter_algorithm=[
    'issue1007-v15-cpdbs-singlecegar-regularplans-pdb1m-pdbs10m-t20-s{}'.format(random_seed) for random_seed in range(2018, 2028)
] + [
    'issue1007-v15-cpdbs-multiplecegar-wildcardplans-pdb1m-pdbs10m-t20-blacklist0.75-stag4-s{}'.format(random_seed) for random_seed in range(2018, 2028)
])

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

    run['score_cegar_computation_time'] = log_score(run.get('cegar_computation_time'), min_bound=1.0, max_bound=MAX_TIME)
    run['score_cpdbs_computation_time'] = log_score(run.get('cpdbs_computation_time'), min_bound=1.0, max_bound=MAX_TIME)
    return run

exp.add_report(
    AverageAlgorithmReport(
        algo_name_suffixes=['-s{}'.format(seed) for seed in range(2018,2028)],
        attributes=[
            'coverage', 'search_time', 'total_time',
            'expansions_until_last_jump', 'score_search_time',
            'score_total_time', 'score_memory', 'score_expansions',
            'initial_h_value', cpdbs_num_patterns,
            cpdbs_total_pdb_size, cpdbs_computation_time,
            score_cpdbs_computation_time, cegar_num_iterations,
            cegar_num_patterns, cegar_total_pdb_size,
            cegar_computation_time, score_cegar_computation_time,
        ],
        filter=[add_computation_time_score],
    ),
    outfile=os.path.join(exp.eval_dir, "average", "properties"),
    name="report-average"
)

exp.add_fetcher(os.path.join(exp.eval_dir, 'average'), merge=True)
exp._configs = [
    IssueConfig('cpdbs-singlecegar-regularplans-pdb1m-pdbs10m-t20', []),
    IssueConfig('cpdbs-multiplecegar-wildcardplans-pdb1m-pdbs10m-t20-blacklist0.75-stag4', []),
]
exp.add_comparison_table_step_for_revision_pairs(
    revision_pairs=[
        ("issue1007-v15", "issue1007-v16"),
        ("issue1007-v15", "issue1007-v17"),
        ("issue1007-v16", "issue1007-v17"),
    ],
    attributes=[
        'coverage', 'search_time', 'total_time',
        'expansions_until_last_jump', 'score_search_time',
        'score_total_time', 'score_memory', 'score_expansions',
        'initial_h_value', cpdbs_num_patterns, cpdbs_total_pdb_size,
        cpdbs_computation_time, score_cpdbs_computation_time,
        cegar_num_iterations, cegar_num_patterns, cegar_total_pdb_size,
        cegar_computation_time, score_cegar_computation_time,
    ],
    filter=[add_computation_time_score],
)

exp.run_steps()
