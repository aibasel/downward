#! /usr/bin/env python
# -*- coding: utf-8 -*-
"""
Before you can run the experiment you need to create duplicates of the
two tasks we want to test:

cd ../benchmarks/trucks-strips
for i in {00..19}; do cp p16.pddl p16-$i.pddl; done
for i in {00..19}; do cp domain_p16.pddl domain_p16-$i.pddl; done

cd ../freecell
for i in {00..19}; do cp probfreecell-11-5.pddl probfreecell-11-5-$i.pddl; done

Don't forget to remove the duplicate tasks afterwards. Otherwise they
will be included in subsequent experiments.
"""

import common_setup


REVS = ["issue544-base", "issue544-v1"]
LIMITS = {"search_time": 1800}
CONFIGS = {
    "eager_greedy_add": [
        "--heuristic",
        "h=add()",
        "--search",
        "eager_greedy(h, preferred=h)"],
    "eager_greedy_ff": [
        "--heuristic",
        "h=ff()",
        "--search",
        "eager_greedy(h, preferred=h)"],
    "lazy_greedy_add": [
        "--heuristic",
        "h=add()",
        "--search",
        "lazy_greedy(h, preferred=h)"],
    "lazy_greedy_ff": [
        "--heuristic",
        "h=ff()",
        "--search",
        "lazy_greedy(h, preferred=h)"],
}

TEST_RUN = False

if TEST_RUN:
    SUITE = "gripper:prob01.pddl"
    PRIORITY = None  # "None" means local experiment
else:
    SUITE = (["trucks-strips:p16-%02d.pddl" % i for i in range(20)] +
             ["freecell:probfreecell-11-5-%02d.pddl" % i for i in range(20)])
    PRIORITY = 0     # number means maia experiment


exp = common_setup.IssueExperiment(
    revisions=REVS,
    configs=CONFIGS,
    suite=SUITE,
    limits=LIMITS,
    )

exp.add_comparison_table_step()

exp()
