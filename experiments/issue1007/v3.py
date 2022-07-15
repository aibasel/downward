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
REVISIONS = ["issue1007-v2", "issue1007-v3"]
random_seed=2018
CONFIGS = [
    ### single cegar
    IssueConfig('cpdbs-single-cegar-allgoals-regularplans-pdb1m-pdbs10m-t100-s{}'.format(random_seed), ['--search', 'astar(cpdbs(single_cegar(max_refinements=infinity,ignore_goal_violations=false,wildcard_plans=false,max_time=100,max_pdb_size=1000000,max_collection_size=10000000,random_seed={},verbosity=silent)),verbosity=silent)'.format(random_seed)]),
    IssueConfig('cpdbs-single-cegar-allgoals-wildcardplans-pdb1m-pdbs10m-t100-s{}'.format(random_seed), ['--search', 'astar(cpdbs(single_cegar(max_refinements=infinity,ignore_goal_violations=false,wildcard_plans=true,max_time=100,max_pdb_size=1000000,max_collection_size=10000000,random_seed={},verbosity=silent)),verbosity=silent)'.format(random_seed)]),
]

SUITE = common_setup.DEFAULT_OPTIMAL_SUITE
ENVIRONMENT = BaselSlurmEnvironment(
    email="silvan.sievers@unibas.ch",
    partition="infai_2",
    export=[],
    setup='export PATH=/scicore/soft/apps/binutils/2.32-GCCcore-8.3.0/bin:/scicore/soft/apps/GCCcore/8.3.0/bin:/infai/sieverss/repos/bin:/infai/sieverss/local:/export/soft/lua_lmod/centos7/lmod/lmod/libexec:$PATH\nexport LD_LIBRARY_PATH=/scicore/soft/apps/binutils/2.32-GCCcore-8.3.0/lib:/scicore/soft/apps/zlib/1.2.11-GCCcore-8.3.0/lib:/scicore/soft/apps/GCCcore/8.3.0/lib64:/scicore/soft/apps/GCCcore/8.3.0/lib')

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

#exp.add_absolute_report_step()
exp.add_comparison_table_step()

exp.run_steps()
