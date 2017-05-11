#! /usr/bin/env python

USAGE = """\
Update baseline:
  * change BASELINE variable below
  * push the change
  * login to buildbot server and become the buildslave user
      sudo -u buildslave -H bash
  * remove ~/experiments dir
  * run in an updated repo (e.g. in ~/lib/downward):
    export PYTHONPATH=~/lib/python/lab
    export DOWNWARD_COIN_ROOT=~/lib/coin
    export DOWNWARD_CPLEX_ROOT=~/lib/cplex/cplex
    cd misc/buildbot
    ./buildbot-exp.py --test nightly --rev baseline --all
    ./buildbot-exp.py --test weekly --rev baseline --all

Compare the current revision to the baseline (add to master.cfg):
  ./buildbot-exp.py --test nightly --all
  ./buildbot-exp.py --test weekly --all

These commands exit with 1 if a regression was found.

You can adapt the experiment by changing the values for BASELINE,
CONFIGS, SUITES and RELATIVE_CHECKS below.

"""

import logging
import os
import shutil

from lab.steps import Step
from lab.experiment import ARGPARSER

from downward import cached_revision
from downward.experiment import FastDownwardExperiment
from downward.reports.absolute import AbsoluteReport

from regression_test import Check, RegressionCheckReport


DIR = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(DIR, '../../'))
BENCHMARKS_DIR = os.path.join(REPO, "misc", "tests", "benchmarks")
EXPERIMENTS_DIR = os.path.expanduser('~/experiments')
CACHE_DIR = os.path.expanduser('~/lab')

BASELINE = cached_revision.get_global_rev(REPO, rev='eb9f8c86918f')
CONFIGS = {}
CONFIGS['nightly'] = [
    ('lmcut', ['--search', 'astar(lmcut())']),
    ('lazy-greedy-ff', ['--heuristic', 'h=ff()', '--search', 'lazy_greedy([h], preferred=[h])']),
    ('lazy-greedy-cea', ['--heuristic', 'h=cea()', '--search', 'lazy_greedy([h], preferred=[h])']),
    ('lazy-greedy-ff-cea', ['--heuristic', 'hff=ff()', '--heuristic',  'hcea=cea()',
                            '--search', 'lazy_greedy([hff, hcea], preferred=[hff, hcea])']),
    ('blind', ['--search', 'astar(blind())']),
    # TODO: Revert to optimal=true.
    ('lmcount-optimal', ['--search',
        'astar(lmcount(lm_merged([lm_rhw(),lm_hm(m=1)]),admissible=true,optimal=false))']),
]
CONFIGS['weekly'] = CONFIGS['nightly']

SUITES = {
    'nightly': ['gripper:prob01.pddl', 'miconic:s1-0.pddl'],
    'weekly': ['gripper:prob01.pddl', 'miconic:s1-0.pddl'],
}

TRANSLATOR_ATTRIBUTES = [
    'auxiliary_atoms', 'axioms', 'derived_variables',
    'effect_conditions_simplified', 'facts', 'final_queue_length',
    'implied_preconditions_added', 'mutex_groups',
    'operators', 'operators_removed', 'propositions_removed',
    'relevant_atoms', 'task_size', 'total_mutex_groups_size',
    'total_queue_pushes', 'uncovered_facts', 'variables']
TIME_ATTRIBUTES = (
    ['search_time', 'total_time'] + ['translator_time_%s' % attr for attr in (
    'building_dictionary_for_full_mutex_groups', 'building_mutex_information',
    'building_strips_to_sas_dictionary', 'building_translation_key',
    'checking_invariant_weight', 'choosing_groups', 'collecting_mutex_groups',
    'completing_instantiation', 'computing_fact_groups', 'computing_model',
    'detecting_unreachable_propositions', 'done', 'finding_invariants',
    'generating_datalog_program', 'instantiating', 'instantiating_groups',
    'normalizing_datalog_program', 'normalizing_task', 'parsing',
    'preparing_model', 'processing_axioms', 'simplifying_axioms',
    'translating_task', 'writing_output')])
SEARCH_ATTRIBUTES = ['dead_ends', 'evaluations', 'expansions',
                     'expansions_until_last_jump', 'generated', 'reopened']
MEMORY_ATTRIBUTES = ['translator_peak_memory', 'memory']
RELATIVE_CHECKS = ([
    Check('initial_h_value', min_rel=1.0),
    Check('cost', max_rel=1.0),
    Check('plan_length', max_rel=1.0)] +
    [Check('translator_%s' % attr, max_rel=1.0) for attr in TRANSLATOR_ATTRIBUTES] +
    [Check(attr, max_rel=1.05, ignored_abs_diff=1) for attr in TIME_ATTRIBUTES] +
    [Check(attr, max_rel=1.05) for attr in SEARCH_ATTRIBUTES] +
    [Check(attr, max_rel=1.05, ignored_abs_diff=1024) for attr in MEMORY_ATTRIBUTES])

# Absolute attributes are reported, but not checked.
ABSOLUTE_ATTRIBUTES = [check.attribute for check in RELATIVE_CHECKS]


def parse_custom_args():
    ARGPARSER.description = USAGE
    ARGPARSER.add_argument('--rev', dest='revision', default='default',
        help='Fast Downward revision or "baseline".')
    ARGPARSER.add_argument('--test', choices=['nightly', 'weekly'], default='nightly',
        help='Select whether "nightly" or "weekly" tests should be run.')
    return ARGPARSER.parse_args()

def get_exp_dir(name, test):
    return os.path.join(EXPERIMENTS_DIR, '%s-%s' % (name, test))

def main():
    args = parse_custom_args()

    if args.revision.lower() == 'baseline':
        rev = BASELINE
        name = 'baseline'
    else:
        rev = args.revision
        name = rev

    exp = FastDownwardExperiment(path=get_exp_dir(name, args.test), cache_dir=CACHE_DIR)
    exp.add_suite(BENCHMARKS_DIR, SUITES[args.test])
    for config_nick, config in CONFIGS[args.test]:
        exp.add_algorithm(rev + "-" + config_nick, REPO, rev, config)
    exp.add_report(AbsoluteReport(attributes=ABSOLUTE_ATTRIBUTES), name='report')

    # Only compare results if we are not running the baseline experiment.
    if rev != BASELINE:
        dirty_paths = [
            path for path in [exp.path, exp.eval_dir]
            if os.path.exists(path)]
        if dirty_paths:
            logging.critical(
                'The last run found a regression. Please inspect what '
                'went wrong and then delete the following directories '
                'manually: %s' % dirty_paths)
        exp.add_fetcher(
            src=get_exp_dir('baseline', args.test) + '-eval',
            dest=exp.eval_dir,
            name='fetch-baseline-results')
        exp.add_report(AbsoluteReport(attributes=ABSOLUTE_ATTRIBUTES), name='comparison')
        exp.add_report(
            RegressionCheckReport(BASELINE, RELATIVE_CHECKS), name='regression-check')
        # We abort if there is a regression and keep the directories.
        exp.add_step(Step('rm-exp-dir', shutil.rmtree, exp.path))
        exp.add_step(Step('rm-eval-dir', shutil.rmtree, exp.eval_dir))

    exp()

main()
