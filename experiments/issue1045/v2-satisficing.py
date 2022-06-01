#! /usr/bin/env python
# -*- coding: utf-8 -*-

import common_setup
import os

from common_setup import IssueConfig, IssueExperiment
from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from lab.reports import Attribute

ISSUE = "issue1045"


def make_comparison_table():
    report = common_setup.ComparativeReport(
        algorithm_pairs=[
            (f"{ISSUE}-base-lama-first", f"{ISSUE}-v2-lama-first"),
            (f"{ISSUE}-base-lama-first-pref", f"{ISSUE}-v2-lama-first-pref"),
            (f"{ISSUE}-base-lm-zg", f"{ISSUE}-v2-lm-zg"),
        ], attributes=ATTRIBUTES,
    )
    outfile = os.path.join(
        exp.eval_dir, "%s-compare.%s" % (exp.name, report.output_format)
    )
    report(exp.eval_dir, outfile)
    exp.add_report(report)

REVISIONS = [
    f"{ISSUE}-base",
    f"{ISSUE}-v2",
]

CONFIGS = [
    IssueConfig("lama-first", [],
                driver_options=["--alias", "lama-first",
                                "--overall-time-limit", "5m"]),
    IssueConfig(
        "lama-first-pref", ["--evaluator",
                            "hlm=lmcount(lm_factory=lm_reasonable_orders_hps("
                            "lm_rhw()),transform=adapt_costs(one),pref=true)",
                            "--evaluator", "hff=ff(transform=adapt_costs(one))",
                            "--search",
                            """lazy_greedy([hff,hlm],preferred=[hff,hlm], cost_type=one,reopen_closed=false)"""],
        driver_options=["--overall-time-limit", "5m"],
    ),
    IssueConfig("lm-zg", ["--search", "eager_greedy([lmcount(lm_zg())])"],
                driver_options=["--overall-time-limit", "5m"]),
]

BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]

if common_setup.is_running_on_cluster():
    SUITE = common_setup.DEFAULT_SATISFICING_SUITE
    ENVIRONMENT = BaselSlurmEnvironment(
        partition="infai_2",
        email="salome.eriksson@unibas.ch",
        setup="export PATH=/scicore/soft/apps/binutils/2.32-GCCcore-8.3.0/bin:/scicore/soft/apps/CMake/3.15.3-GCCcore-8.3.0/bin:/scicore/soft/apps/cURL/7.66.0-GCCcore-8.3.0/bin:/scicore/soft/apps/bzip2/1.0.8-GCCcore-8.3.0/bin:/scicore/soft/apps/ncurses/6.1-GCCcore-8.3.0/bin:/scicore/soft/apps/GCCcore/8.3.0/bin:/infai/buecle01/local:/export/soft/lua_lmod/centos7/lmod/lmod/libexec:/usr/local/bin:/usr/bin:/usr/local/sbin:/usr/sbin:$PATH\nexport LD_LIBRARY_PATH=/scicore/soft/apps/binutils/2.32-GCCcore-8.3.0/lib:/scicore/soft/apps/cURL/7.66.0-GCCcore-8.3.0/lib:/scicore/soft/apps/bzip2/1.0.8-GCCcore-8.3.0/lib:/scicore/soft/apps/zlib/1.2.11-GCCcore-8.3.0/lib:/scicore/soft/apps/ncurses/6.1-GCCcore-8.3.0/lib:/scicore/soft/apps/GCCcore/8.3.0/lib64:/scicore/soft/apps/GCCcore/8.3.0/lib",
    )
else:
    SUITE = common_setup.IssueExperiment.DEFAULT_TEST_SUITE
    ENVIRONMENT = LocalEnvironment(processes=2)

exp = common_setup.IssueExperiment(
    revisions=REVISIONS,
    configs=CONFIGS,
    environment=ENVIRONMENT,
)

exp.add_suite(BENCHMARKS_DIR, SUITE)

exp.add_parser(exp.ANYTIME_SEARCH_PARSER)
exp.add_parser(exp.EXITCODE_PARSER)
exp.add_parser(exp.PLANNER_PARSER)
exp.add_parser(exp.SINGLE_SEARCH_PARSER)
exp.add_parser("landmark_parser.py")

ATTRIBUTES = IssueExperiment.DEFAULT_TABLE_ATTRIBUTES + [
    Attribute("landmarks", min_wins=False),
    Attribute("landmarks_disjunctive", min_wins=False),
    Attribute("landmarks_conjunctiv", min_wins=False),
    Attribute("orderings", min_wins=False),
    Attribute("lmgraph_generation_time", min_wins=True),
]

exp.add_step("build", exp.build)
exp.add_step("start", exp.start_runs)
exp.add_fetcher(name="fetch")
exp.add_step("comparison table", make_comparison_table)
exp.add_parse_again_step()

exp.run_steps()

