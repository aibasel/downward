#! /usr/bin/env python

"""
Usage:

# Run experiment manually for the baseline revision.
./buildbot-exp.py --test weekly --rev baseline --all

# Let the buildbot run the following for every changeset (bundle).
./buildbot-exp.py --test weekly --all
"""

import logging
import os
import sys

from lab.fetcher import Fetcher
from lab.steps import Step
from lab.tools import ArgParser
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
CONFIGS = {
    'nightly': [('lmcut', ['--search', 'astar(lmcut())'])],
    'weekly': [('lmcut', ['--search', 'astar(lmcut())'])],
}
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
    Check('memory', max_rel=1.05, max_abs_diff=1024),
    Check('translator_time_done', max_rel=1.05, max_abs_diff=1),
]


# TODO: Experiment.argparser -> lab.experiment.argparser (make it globally available)
#       and let users add custom arguments after the normal args (and a dummy epilog)
#       have been added.
class ConsumingArgParser(ArgParser):
    def __init__(self, *args, **kwargs):
        ArgParser.__init__(self, *args, add_help=False)

    def parse_args(self, *args, **kwargs):
        args, remaining = self.parse_known_args(*args, **kwargs)
        sys.argv = [sys.argv[0]] + remaining
        return args

def parse_custom_args():
    parser = ConsumingArgParser()
    # TODO: Add help strings.
    parser.add_argument('--rev', dest='revision')
    parser.add_argument('--test', choices=['nightly', 'weekly'], default='nightly')
    return parser.parse_args()

def get_exp_dir(rev, test):
    return os.path.join(DIR, 'experiments', '%s-%s' % (rev, test))

args = parse_custom_args()

if not args.revision:
    rev = checkouts.get_global_rev(REPO)
elif args.revision.lower() == 'baseline':
    rev = BASELINE
else:
    rev = checkouts.get_global_rev(REPO, args.revision)

combo = [(Translator(REPO, rev=rev), Preprocessor(REPO, rev=rev), Planner(REPO, rev=rev))]

exp = DownwardExperiment(path=get_exp_dir(rev, args.test),
                         repo=REPO,
                         combinations=combo)
exp.add_suite(SUITES[args.test])
for nick, config in CONFIGS[args.test]:
    exp.add_config(nick, config)
exp.add_report(AbsoluteReport(attributes=ABSOLUTE_ATTRIBUTES))

# Only compare results if we are not running the baseline experiment.
if rev != BASELINE:
    logging.info('Testing revision %s' % rev)
    exp.add_step(Step('fetch-baseline-results', Fetcher(),
                      get_exp_dir(BASELINE, args.test) + '-eval',
                      exp.eval_dir))
    exp.add_report(AbsoluteReport())
    exp.add_report(RegressionCheckReport(BASELINE, RELATIVE_ATTRIBUTE_CHECKS))
    exp.add_step(Step.remove_exp_dir(exp))

exp()
