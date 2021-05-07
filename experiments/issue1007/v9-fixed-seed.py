#! /usr/bin/env python3

import itertools
import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment

from downward.reports.compare import ComparativeReport

import common_setup
from common_setup import IssueConfig, IssueExperiment

DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPT_NAME = os.path.splitext(os.path.basename(__file__))[0]
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue1007-v8c", "issue1007-v9"]
random_seed=2018
MAX_TIME=100
if common_setup.is_test_run():
    MAX_TIME=2
CONFIGS = [
    ### ipdb
    IssueConfig(f'cpdbs-hillclimbing-pdb1m-pdbs10m-t100-s{random_seed}', ['--search', f'astar(cpdbs(hillclimbing(max_time={MAX_TIME},pdb_max_size=1000000,collection_max_size=10000000,random_seed={random_seed})),verbosity=silent)']),

    ### single cegar
    IssueConfig(f'cpdbs-singlecegar-regularplans-pdb1m-pdbs10m-t100-s{random_seed}', ['--search', f'astar(cpdbs(single_cegar(wildcard_plans=false,max_time={MAX_TIME},max_pdb_size=1000000,max_collection_size=10000000,random_seed={random_seed},verbosity=normal)),verbosity=silent)']),
    IssueConfig(f'cpdbs-singlecegar-wildcardplans-pdb1m-pdbs10m-t100-s{random_seed}', ['--search', f'astar(cpdbs(single_cegar(wildcard_plans=true,max_time={MAX_TIME},max_pdb_size=1000000,max_collection_size=10000000,random_seed={random_seed},verbosity=normal)),verbosity=silent)']),

    ### multiple cegar
    IssueConfig(f'cpdbs-multiplecegar-regularplans-pdb1m-pdbs10m-t100-s{random_seed}', ['--search', f"astar(cpdbs(multiple_cegar(wildcard_plans=false,max_time=100,max_pdb_size=1000000,max_collection_size=10000000,random_seed={random_seed},max_collection_size=10000000,total_max_time={MAX_TIME},stagnation_limit=infinity,blacklist_trigger_percentage=1,blacklist_on_stagnation=false,verbosity=normal)),verbosity=silent)"]),
    IssueConfig(f'cpdbs-multiplecegar-regularplans-pdb1m-pdbs10m-t100-blacklist0.75-stag20-s{random_seed}', ['--search', f"astar(cpdbs(multiple_cegar(wildcard_plans=false,max_time=100,max_pdb_size=1000000,max_collection_size=10000000,random_seed={random_seed},max_collection_size=10000000,total_max_time={MAX_TIME},stagnation_limit=20,blacklist_trigger_percentage=0.75,blacklist_on_stagnation=true,verbosity=normal)),verbosity=silent)"]),

    IssueConfig(f'cpdbs-multiplecegar-wildcardplans-pdb1m-pdbs10m-t100-s{random_seed}', ['--search', f"astar(cpdbs(multiple_cegar(wildcard_plans=true,max_time=100,max_pdb_size=1000000,max_collection_size=10000000,random_seed={random_seed},max_collection_size=10000000,total_max_time={MAX_TIME},stagnation_limit=infinity,blacklist_trigger_percentage=1,blacklist_on_stagnation=false,verbosity=normal)),verbosity=silent)"]),
    IssueConfig(f'cpdbs-multiplecegar-wildcardplans-pdb1m-pdbs10m-t100-blacklist0.75-stag20-s{random_seed}', ['--search', f"astar(cpdbs(multiple_cegar(wildcard_plans=true,max_time=100,max_pdb_size=1000000,max_collection_size=10000000,random_seed={random_seed},max_collection_size=10000000,total_max_time={MAX_TIME},stagnation_limit=20,blacklist_trigger_percentage=0.75,blacklist_on_stagnation=true,verbosity=normal)),verbosity=silent)"]),
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

exp.add_absolute_report_step()
exp.add_comparison_table_step()

exp.run_steps()
