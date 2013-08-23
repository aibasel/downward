#! /usr/bin/env python

"""
Usage:

# Run experiment manually for the baseline revision.
./buildbot-exp.py --test weekly --rev baseline --all

# Let the buildbot run the following for every changeset (bundle).
./buildbot-exp.py --test weekly --all
"""

import os
import shutil

from lab.fetcher import Fetcher
from lab.steps import Step
from lab.experiment import ARGPARSER
from downward.experiments import DownwardExperiment
# TODO: Use add_revision() once it's available.
from downward.checkouts import Translator, Preprocessor, Planner
from downward import checkouts
from downward import configs
from downward.reports.absolute import AbsoluteReport

from check import Check, RegressionCheckReport


DIR = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.join(DIR, '../../')

# TODO: Set baseline, configs, suites and attributes.
BASELINE = checkouts.get_global_rev(REPO, '3222')
CONFIGS = {}
CONFIGS['nightly'] = [
    ('lmcut', ['--search', 'astar(lmcut())']),
    ('lazy-greedy-ff', ['--heuristic', 'h=ff()', '--search', 'lazy_greedy(h, preferred=h)']),
    ('lazy-greedy-cea', ['--heuristic', 'h=cea()', '--search', 'lazy_greedy(h, preferred=h)']),
    ('lazy-greedy-ff-cea', ['--heuristic', 'hff=ff()', '--heuristic',  'hcea=cea()',
                            '--search', 'lazy_greedy([hff, hcea], preferred=[hff, hcea])']),
    ('lama-2011', ['ipc', 'seq-sat-lama-2011']),
    ('blind', ['--search', 'astar(blind())']),
    ('merge-and-shrink-bisim', ['--search',
        'astar(merge_and_shrink(merge_strategy=merge_linear_reverse_level,'
            'shrink_strategy=shrink_bisimulation(max_states=50000,greedy=false,'
            'group_by_h=true)))']),
]
CONFIGS['weekly'] = CONFIGS['nightly']

SUITES = {
    'nightly': ['gripper:prob01.pddl', 'blocks:probBLOCKS-4-1.pddl'],
    'weekly': ['gripper:prob01.pddl', 'gripper:prob02.pddl',
               'blocks:probBLOCKS-4-1.pddl', 'blocks:probBLOCKS-4-2.pddl'],
}
ABSOLUTE_ATTRIBUTES = ['coverage', 'expansions']
RELATIVE_ATTRIBUTE_CHECKS = [
    Check('expansions', max_rel=1.05),
    Check('initial_h_value', min_rel=1.0),
    Check('cost', max_rel=0.5),
    Check('search_time', max_rel=1.05),
    Check('total_time', max_rel=1.05),
    Check('memory', max_rel=1.05, ignored_abs_diff=1024),
    Check('translator_time_done', max_rel=1.05, ignored_abs_diff=1),
]

def parse_custom_args():
    ARGPARSER.add_argument('--rev', dest='revision',
        help='Fast Downward revision or "baseline". If omitted use current revision.')
    ARGPARSER.add_argument('--test', choices=['nightly', 'weekly'], default='nightly',
        help='Select whether "nightly" or "weekly" tests should be run.')
    return ARGPARSER.parse_args()

def get_exp_dir(rev, test):
    return os.path.join(DIR, 'experiments', '%s-%s' % (rev, test))

def main():
    args = parse_custom_args()

    if not args.revision:
        # If the working directory contains changes, the revision ends with '+'.
        # Strip this to use the vanilla revision.
        rev = checkouts.get_global_rev(REPO).rstrip('+')
    elif args.revision.lower() == 'baseline':
        rev = BASELINE
    else:
        rev = checkouts.get_global_rev(REPO, args.revision)

    combo = [(Translator(REPO, rev=rev), Preprocessor(REPO, rev=rev), Planner(REPO, rev=rev))]

    exp = DownwardExperiment(path=get_exp_dir(rev, args.test), repo=REPO, combinations=combo)
    exp.add_suite(SUITES[args.test])
    for nick, config in CONFIGS[args.test]:
        exp.add_config(nick, config)
    exp.add_report(AbsoluteReport(attributes=ABSOLUTE_ATTRIBUTES), name='report')

    # Only compare results if we are not running the baseline experiment.
    if rev != BASELINE:
        exp.add_step(Step('fetch-baseline-results', Fetcher(),
                          get_exp_dir(BASELINE, args.test) + '-eval',
                          exp.eval_dir))
        exp.add_report(AbsoluteReport(attributes=ABSOLUTE_ATTRIBUTES), name='comparison')
        exp.add_step(Step('rm-preprocess-dir', shutil.rmtree, exp.preprocess_exp_path, ignore_errors=True))
        exp.add_step(Step('rm-exp-dir', shutil.rmtree, exp.path, ignore_errors=True))
        exp.add_report(RegressionCheckReport(BASELINE, RELATIVE_ATTRIBUTE_CHECKS),
                       name='regression-check')

    exp()

main()
