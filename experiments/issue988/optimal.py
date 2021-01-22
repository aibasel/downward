#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os
import subprocess

from downward.reports.absolute import AbsoluteReport
from lab.experiment import Experiment


ATTRIBUTES = [
    "cost",
    "coverage",
    "error",
    "evaluations",
    "expansions",
    "expansions_until_last_jump",
    "initial_h_value",
    "generated",
    "memory",
    "planner_memory",
    "planner_time",
    "quality",
    "run_dir",
    "score_evaluations",
    "score_expansions",
    "score_generated",
    "score_memory",
    "score_search_time",
    "score_total_time",
    "search_time",
    "total_time",
]

EXPERIMENTS_DIRS = [
    "data/issue988-210121_optimal-eval",
    "data/issue988-210122_optimal-eval",
]

exp_name = "optimal"
exp_dir_name = f"data/{exp_name}"
os.makedirs(f"{exp_dir_name}-eval", exist_ok=True)

first = True
exp = Experiment(exp_dir_name)
for exp_dir in EXPERIMENTS_DIRS:
    exp.add_fetcher(src=exp_dir, merge=not first)
    first = False

report = AbsoluteReport(attributes=ATTRIBUTES)
outfile = os.path.join(exp.eval_dir, "report-optimal")
exp.add_report(report, outfile)

exp.add_step(f"publish-{outfile}", subprocess.call, 
             ["publish", f"{outfile}.html"])

exp.run_steps()

