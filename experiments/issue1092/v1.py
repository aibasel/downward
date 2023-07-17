#! /usr/bin/env python3

import itertools
import os
from pathlib import Path

from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from lab.reports import Attribute, geometric_mean

from downward.reports.compare import ComparativeReport

import common_setup
from common_setup import IssueConfig, IssueExperiment

ARCHIVE_PATH = "ai/downward/issue1092"
DIR = os.path.dirname(os.path.abspath(__file__))
REPO_DIR = os.environ["DOWNWARD_REPO"]
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISION = "issue1092-v1"
REVISIONS = [REVISION]
BUILDS = ["release"]
CONFIG_NICKS = [
    ('sbmiasm-b50k', ['--search', 'astar(merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false),merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[sf_miasm(use_caching=false,shrink_strategy=shrink_bisimulation(greedy=false),max_states=50000,threshold_before_merge=1),total_order(atomic_ts_order=reverse_level,product_ts_order=new_to_old,atomic_before_product=false)])),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))']),
    ('sbmiasm-b50k-cache', ['--search', 'astar(merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false),merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[sf_miasm(use_caching=true,shrink_strategy=shrink_bisimulation(greedy=false),max_states=50000,threshold_before_merge=1),total_order(atomic_ts_order=reverse_level,product_ts_order=new_to_old,atomic_before_product=false)])),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))']),
    ('sccs-sbmiasm-b50k', ['--search', 'astar(merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false),merge_strategy=merge_sccs(order_of_sccs=topological,merge_selector=score_based_filtering(scoring_functions=[sf_miasm(use_caching=false,shrink_strategy=shrink_bisimulation(greedy=false),max_states=50000,threshold_before_merge=1),total_order(atomic_ts_order=reverse_level,product_ts_order=new_to_old,atomic_before_product=false)])),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))']),
    ('sccs-sbmiasm-b50k-cache', ['--search', 'astar(merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false),merge_strategy=merge_sccs(order_of_sccs=topological,merge_selector=score_based_filtering(scoring_functions=[sf_miasm(use_caching=true,shrink_strategy=shrink_bisimulation(greedy=false),max_states=50000,threshold_before_merge=1),total_order(atomic_ts_order=reverse_level,product_ts_order=new_to_old,atomic_before_product=false)])),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))']),
]
CONFIGS = [
    IssueConfig(
        config_nick,
        config,
        build_options=[build],
        driver_options=["--build", build])
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
    setup='export PATH=/scicore/soft/apps/binutils/2.36.1-GCCcore-10.3.0/bin:/scicore/soft/apps/CMake/3.23.1-GCCcore-10.3.0/bin:/scicore/soft/apps/OpenSSL/1.1/bin:/scicore/soft/apps/libarchive/3.5.1-GCCcore-10.3.0/bin:/scicore/soft/apps/XZ/5.2.5-GCCcore-10.3.0/bin:/scicore/soft/apps/cURL/7.76.0-GCCcore-10.3.0/bin:/scicore/soft/apps/bzip2/1.0.8-GCCcore-10.3.0/bin:/scicore/soft/apps/ncurses/6.2-GCCcore-10.3.0/bin:/scicore/soft/apps/GCCcore/10.3.0/bin:/infai/sieverss/repos/bin:/infai/sieverss/local:/export/soft/lua_lmod/centos7/lmod/lmod/libexec:/usr/local/bin:/usr/bin:/usr/local/sbin:/usr/sbin:$PATH\nexport LD_LIBRARY_PATH=/scicore/soft/apps/binutils/2.36.1-GCCcore-10.3.0/lib:/scicore/soft/apps/OpenSSL/1.1/lib:/scicore/soft/apps/libarchive/3.5.1-GCCcore-10.3.0/lib:/scicore/soft/apps/XZ/5.2.5-GCCcore-10.3.0/lib:/scicore/soft/apps/cURL/7.76.0-GCCcore-10.3.0/lib:/scicore/soft/apps/bzip2/1.0.8-GCCcore-10.3.0/lib:/scicore/soft/apps/zlib/1.2.11-GCCcore-10.3.0/lib:/scicore/soft/apps/ncurses/6.2-GCCcore-10.3.0/lib:/scicore/soft/apps/GCCcore/10.3.0/lib64')

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
exp.add_parser('ms-parser.py')

exp.add_step('build', exp.build)
exp.add_step('start', exp.start_runs)
exp.add_fetcher(name='fetch')

# planner outcome attributes
perfect_heuristic = Attribute('perfect_heuristic', absolute=True, min_wins=False)

# m&s attributes
ms_construction_time = Attribute('ms_construction_time', absolute=False, min_wins=True, function=geometric_mean)
score_ms_construction_time = Attribute('score_ms_construction_time', min_wins=False, digits=4)
ms_atomic_construction_time = Attribute('ms_atomic_construction_time', absolute=False, min_wins=True, functions=[geometric_mean])
ms_abstraction_constructed = Attribute('ms_abstraction_constructed', absolute=True, min_wins=False)
ms_atomic_fts_constructed = Attribute('ms_atomic_fts_constructed', absolute=True, min_wins=False)
ms_out_of_memory = Attribute('ms_out_of_memory', absolute=True, min_wins=True)
ms_out_of_time = Attribute('ms_out_of_time', absolute=True, min_wins=True)
search_out_of_memory = Attribute('search_out_of_memory', absolute=True, min_wins=True)
search_out_of_time = Attribute('search_out_of_time', absolute=True, min_wins=True)

extra_attributes = [
    perfect_heuristic,

    ms_construction_time,
    score_ms_construction_time,
    ms_atomic_construction_time,
    ms_abstraction_constructed,
    ms_atomic_fts_constructed,
    ms_out_of_memory,
    ms_out_of_time,
    search_out_of_memory,
    search_out_of_time,
]
attributes = exp.DEFAULT_TABLE_ATTRIBUTES
attributes.extend(extra_attributes)

report_name=f'{exp.name}-compare'
report_file=Path(exp.eval_dir) / f'{report_name}.html'
exp.add_report(
    ComparativeReport(
        attributes=attributes,
        algorithm_pairs=[
            (f"{REVISION}-sbmiasm-b50k", f"{REVISION}-sbmiasm-b50k-cache"),
            (f"{REVISION}-sccs-sbmiasm-b50k", f"{REVISION}-sccs-sbmiasm-b50k-cache"),
        ],
    ),
    name=report_name,
    outfile=report_file,
)

exp.add_archive_step(ARCHIVE_PATH)

exp.run_steps()
