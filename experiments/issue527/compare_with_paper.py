#! /usr/bin/env python
# -*- coding: utf-8 -*-

from lab.experiment import Experiment
from lab.steps import Step
from downward.reports.compare import CompareConfigsReport
from common_setup import get_experiment_name, get_data_dir, get_repo_base

import os

DATADIR = os.path.join(os.path.dirname(__file__), 'data')

exp = Experiment(get_data_dir())

exp.add_fetcher(os.path.join(DATADIR, 'e2013101802-pho-seq-constraints-eval'), filter_config_nick="astar_pho_seq_no_onesafe")
exp.add_fetcher(os.path.join(DATADIR, 'issue527-v2-eval'), filter_config_nick="astar_occ_seq")

exp.add_report(CompareConfigsReport(
    [
     ('869fec6f843b-astar_pho_seq_no_onesafe', 'issue527-v2-astar_occ_seq'),
    ],
    attributes=[
                'coverage',
                'total_time',
                'expansions',
                'evaluations',
                'generated',
                'expansions_until_last_jump',
                'error',
                ],
    )
)



exp()
