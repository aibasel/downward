#! /usr/bin/env python3

import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from lab.reports import Attribute

import common_setup
from common_setup import IssueConfig, IssueExperiment

ISSUE = "issue1036"
ARCHIVE_PATH = f"ai/downward/{ISSUE}"
DIR = os.path.dirname(os.path.abspath(__file__))
REPO_DIR = os.environ["DOWNWARD_REPO"]
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = [
    f"{ISSUE}-v11",
]
BUILDS = ["release"]


def get_lm_factory(factory, use_reasonable):
    if use_reasonable:
        return f"lm_reasonable_orders_hps({factory})"
    else:
        return factory


def get_config_name(name, use_reasonable):
    if use_reasonable:
        return f"{name}-reasonable"
    else:
        return name


def make_comparison_table():
    pairs = [
        (f"{ISSUE}-v9-lm-cut", f"{ISSUE}-v9-lm-cut"),
    ] + [
        (f"{ISSUE}-v9-{lm_factory}", f"{ISSUE}-v9-{lm_factory}-reasonable")
        for lm_factory in [
            "seq-opt-bjolp",
            "seq-opt-bjolp-opt",
            "lm-exhaust",
            "lm-hm2",
            "lm-rhw",
            "lm-zg",
        ]
    ]

    report = common_setup.ComparativeReport(
        algorithm_pairs=pairs, attributes=ATTRIBUTES,
    )
    outfile = os.path.join(
        exp.eval_dir, "%s-compare.%s" % (exp.name, report.output_format)
    )
    report(exp.eval_dir, outfile)

    exp.add_report(report)


CONFIG_NICKS = [
    ("lm-cut", ["--search", "astar(lmcut())"], []),
]
for use_reasonable in [True, False]:
    CONFIG_NICKS += [
        (get_config_name("seq-opt-bjolp", use_reasonable),
         ["--evaluator",
          f"lmc=landmark_cost_partitioning({get_lm_factory('lm_merged([lm_rhw(),lm_hm(m=1)])', use_reasonable)})",
          "--search", "astar(lmc,lazy_evaluator=lmc)"], []),
        (get_config_name("seq-opt-bjolp-opt", use_reasonable),
         ["--evaluator",
          f"lmc=landmark_cost_partitioning({get_lm_factory('lm_merged([lm_rhw(),lm_hm(m=1)])', use_reasonable)},optimal=true,lpsolver=CPLEX)",
          "--search", "astar(lmc,lazy_evaluator=lmc)"], []),
        (get_config_name("lm-exhaust", use_reasonable),
         ["--evaluator",
          f"lmc=landmark_cost_partitioning({get_lm_factory('lm_exhaust()', use_reasonable)})",
          "--search", "astar(lmc,lazy_evaluator=lmc)"], []),
        (get_config_name("lm-hm2", use_reasonable),
         ["--evaluator",
          f"lmc=landmark_cost_partitioning({get_lm_factory('lm_hm(m=2)', use_reasonable)})",
          "--search", "astar(lmc,lazy_evaluator=lmc)"], []),
        (get_config_name("lm-rhw", use_reasonable),
         ["--evaluator",
          f"lmc=landmark_cost_partitioning({get_lm_factory('lm_rhw()', use_reasonable)})",
          "--search", "astar(lmc)"], []),
        (get_config_name("lm-zg", use_reasonable),
         ["--evaluator",
          f"lmc=landmark_cost_partitioning({get_lm_factory('lm_zg()', use_reasonable)})",
          "--search", "astar(lmc)"], []),
    ]
CONFIGS = [
    IssueConfig(
        config_nick,
        config,
        build_options=[build],
        driver_options=driver_opts,
    )
    for build in BUILDS
    for config_nick, config, driver_opts in CONFIG_NICKS
]

SUITE = common_setup.DEFAULT_SATISFICING_SUITE
ENVIRONMENT = BaselSlurmEnvironment(
    partition="infai_2",
    email="clemens.buechner@unibas.ch",
    export=["PATH"])
"""
If your experiments sometimes have GCLIBX errors, you can use the
below "setup" parameter instead of the above use "export" parameter for
setting environment variables which "load" the right modules. ("module
load" doesn't do anything else than setting environment variables.)

paths obtained via:
$ module purge
$ module -q load CMake/3.15.3-GCCcore-8.3.0
$ module -q load GCC/8.3.0
$ echo $PATH
$ echo $LD_LIBRARY_PATH
"""
# setup='export PATH=/scicore/soft/apps/binutils/2.32-GCCcore-8.3.0/bin:/scicore/soft/apps/CMake/3.15.3-GCCcore-8.3.0/bin:/scicore/soft/apps/cURL/7.66.0-GCCcore-8.3.0/bin:/scicore/soft/apps/bzip2/1.0.8-GCCcore-8.3.0/bin:/scicore/soft/apps/ncurses/6.1-GCCcore-8.3.0/bin:/scicore/soft/apps/GCCcore/8.3.0/bin:/infai/sieverss/repos/bin:/infai/sieverss/local:/export/soft/lua_lmod/centos7/lmod/lmod/libexec:/usr/local/bin:/usr/bin:/usr/local/sbin:/usr/sbin:$PATH\nexport LD_LIBRARY_PATH=/scicore/soft/apps/binutils/2.32-GCCcore-8.3.0/lib:/scicore/soft/apps/cURL/7.66.0-GCCcore-8.3.0/lib:/scicore/soft/apps/bzip2/1.0.8-GCCcore-8.3.0/lib:/scicore/soft/apps/zlib/1.2.11-GCCcore-8.3.0/lib:/scicore/soft/apps/ncurses/6.1-GCCcore-8.3.0/lib:/scicore/soft/apps/GCCcore/8.3.0/lib64:/scicore/soft/apps/GCCcore/8.3.0/lib'

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
exp.add_parser(exp.SINGLE_SEARCH_PARSER)
exp.add_parser(exp.PLANNER_PARSER)
exp.add_parser("landmark_parser.py")

ATTRIBUTES = IssueExperiment.DEFAULT_TABLE_ATTRIBUTES + [
    Attribute("landmarks", min_wins=False),
    Attribute("landmarks_disjunctive", min_wins=False),
    Attribute("landmarks_conjunctive", min_wins=False),
    Attribute("orderings", min_wins=False),
    Attribute("lmgraph_generation_time"),
]

exp.add_step("build", exp.build)
exp.add_step("start", exp.start_runs)
exp.add_fetcher(name="fetch")

exp.add_step("comparison table", make_comparison_table)
exp.add_scatter_plot_step(relative=True, attributes=["total_time", "memory"])

exp.add_archive_step(ARCHIVE_PATH)
# exp.add_archive_eval_dir_step(ARCHIVE_PATH)

exp.run_steps()
