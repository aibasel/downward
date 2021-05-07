#! /usr/bin/env python3

import itertools
import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment

from downward.reports.compare import ComparativeReport

import common_setup
from common_setup import IssueConfig, IssueExperiment
from average_report import AverageAlgorithmReport

DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPT_NAME = os.path.splitext(os.path.basename(__file__))[0]
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue1007-v7"]
CONFIGS = []
for random_seed in range(2018, 2028):
    ### single cegar
    CONFIGS.append(IssueConfig('cpdbs-singlecegar-regularplans-pdb1m-pdbs10m-t100-s{}'.format(random_seed), ['--search', 'astar(cpdbs(single_cegar(max_refinements=infinity,wildcard_plans=false,max_time=20,max_pdb_size=1000000,max_collection_size=10000000,random_seed={},verbosity=normal)),verbosity=silent)'.format(random_seed)], driver_options=['--search-time-limit', '5m']))

    ### multiple cegar
    CONFIGS.append(IssueConfig('cpdbs-multiplecegar-wildcardplans-pdb1m-pdbs10m-t100-blacklist0.75-stag4-s{}'.format(random_seed), ['--search', "astar(cpdbs(multiple_cegar(max_refinements=infinity,wildcard_plans=true,max_time=100,max_pdb_size=1000000,max_collection_size=10000000,random_seed={},total_max_time=20,stagnation_limit=4,blacklist_trigger_percentage=0.75,blacklist_on_stagnation=true,verbosity=normal)),verbosity=silent)".format(random_seed)], driver_options=['--search-time-limit', '5m']))

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

exp.add_absolute_report_step(attributes=['coverage'])

### compare against v6
exp.add_fetcher('data/issue1007-v6-multiple-seeds-eval',merge=True,filter_algorithm=[
    'issue1007-v6-cpdbs-singlecegar-regularplans-pdb1m-pdbs10m-t100-s{}'.format(random_seed) for random_seed in range(2018, 2028)
] + [
    'issue1007-v6-cpdbs-multiplecegar-wildcardplans-pdb1m-pdbs10m-t100-blacklist0.75-stag4-s{}'.format(random_seed) for random_seed in range(2018, 2028)
])

exp.add_report(
    AverageAlgorithmReport(
        algo_name_suffixes=['-s{}'.format(seed) for seed in range(2018,2028)],
        attributes=['coverage', 'search_time', 'total_time',
        'expansions_until_last_jump'],
        filter_algorithm=[
            'issue1007-v6-cpdbs-singlecegar-regularplans-pdb1m-pdbs10m-t100-s{}'.format(random_seed) for random_seed in range(2018, 2028)
        ] + [
            'issue1007-v6-cpdbs-multiplecegar-wildcardplans-pdb1m-pdbs10m-t100-blacklist0.75-stag4-s{}'.format(random_seed) for random_seed in range(2018, 2028)
        ] + [
            'issue1007-v7-cpdbs-singlecegar-regularplans-pdb1m-pdbs10m-t100-s{}'.format(random_seed) for random_seed in range(2018, 2028)
        ] + [
            'issue1007-v7-cpdbs-multiplecegar-wildcardplans-pdb1m-pdbs10m-t100-blacklist0.75-stag4-s{}'.format(random_seed) for random_seed in range(2018, 2028)
        ],
    ),
    outfile=os.path.join(exp.eval_dir, "average-v6-v7", "properties"),
    name="report-average-v6-v7"
)

exp.add_fetcher('data/issue1007-v7-multiple-seeds-eval/average-v6-v7', merge=True)
exp._configs = [
    IssueConfig('cpdbs-singlecegar-regularplans-pdb1m-pdbs10m-t100', []),
    IssueConfig('cpdbs-multiplecegar-wildcardplans-pdb1m-pdbs10m-t100-blacklist0.75-stag4', []),
]
exp.add_comparison_table_step(
    attributes=['coverage', 'search_time', 'total_time',
    'expansions_until_last_jump'],
    name="compare-v6-v7",
    revisions=["issue1007-v6", "issue1007-v7"],
)

exp.run_steps()
