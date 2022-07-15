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
REVISIONS = ["issue1007-v1", "issue1007-v2"]
CONFIGS = [
    IssueConfig('cpdbs-multiple-cegar-wildcardplans-pdb1m-pdbs10m-t100-blacklist0.75-stag20', []),
]

exp = IssueExperiment(
    revisions=REVISIONS,
    configs=CONFIGS,
)

exp.add_comparison_table_step(
    attributes=['coverage', 'search_time', 'total_time', 'expansions_until_last_jump']
)

exp.run_steps()
