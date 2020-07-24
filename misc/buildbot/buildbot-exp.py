#! /usr/bin/env python3

USAGE = """\
1) Use via buildbot:

The buildbot weekly and nightly tests use this script to check for
performance regressions. To update the baseline:
  * change BASELINE variable below
  * push the change
  * login to http://buildbot.fast-downward.org
  * Under Builds > Builders > recreate-baseline-worker-gcc8-lp select
    "force-recreate-baseline". Make sure to "force" a new build instead
    of "rebuilding" an existing build. Rebuilding will regenerate the
    old baseline.
  * Wait for the next nightly build or force a nightly build (do not
    rebuild an old build).

  You can find the experiment data on the Linux build slave in the
  docker volume "buildbot-experiments".


2) Use as commandline tool:

Create baseline data
  ./buildbot-exp.py --test nightly --rev baseline --all
  ./buildbot-exp.py --test weekly --rev baseline --all

Compare the current revision to the baseline (these commands exit
with 1 if a regression was found):
  ./buildbot-exp.py --test nightly --all
  ./buildbot-exp.py --test weekly --all

You can adapt the experiment by changing the values for BASELINE,
CONFIGS, SUITES and RELATIVE_CHECKS below.

"""

import logging
import os
import shutil
import subprocess
import sys

from lab.experiment import ARGPARSER
from lab import cached_revision, tools

from downward.experiment import FastDownwardExperiment
from downward.reports.absolute import AbsoluteReport

from regression_test import Check, RegressionCheckReport


DIR = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(DIR, '../../'))
BENCHMARKS_DIR = os.path.join(REPO, "misc", "tests", "benchmarks")
DEFAULT_BASE_DIR = os.path.dirname(tools.get_script_path())
BASE_DIR = os.getenv("BUILDBOT_EXP_BASE_DIR", DEFAULT_BASE_DIR)
EXPERIMENTS_DIR = os.path.join(BASE_DIR, 'data')
REVISION_CACHE = os.path.join(BASE_DIR, 'revision-cache')
REGRESSIONS_DIR = os.path.join(BASE_DIR, 'regressions')

BASELINE = '0b4344f8f5a8'
CONFIGS = {}
CONFIGS['nightly'] = [
    ('lmcut', ['--search', 'astar(lmcut())']),
    ('lazy-greedy-ff', ['--evaluator', 'h=ff()', '--search', 'lazy_greedy([h], preferred=[h])']),
    ('lazy-greedy-cea', ['--evaluator', 'h=cea()', '--search', 'lazy_greedy([h], preferred=[h])']),
    ('lazy-greedy-ff-cea', ['--evaluator', 'hff=ff()', '--heuristic',  'hcea=cea()',
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
    ARGPARSER.add_argument('--rev', dest='revision', default='main',
        help='Fast Downward revision or "baseline".')
    ARGPARSER.add_argument('--test', choices=['nightly', 'weekly'], default='nightly',
        help='Select whether "nightly" or "weekly" tests should be run.')
    return ARGPARSER.parse_args()

def get_exp_dir(name, test):
    return os.path.join(EXPERIMENTS_DIR, '%s-%s' % (name, test))

def regression_test_handler(test, rev, success):
    if not success:
        tools.makedirs(REGRESSIONS_DIR)
        tarball = os.path.join(REGRESSIONS_DIR, "{test}-{rev}.tar.gz".format(**locals()))
        subprocess.check_call(
            ["tar", "-czf", tarball, "-C", BASE_DIR, os.path.relpath(EXPERIMENTS_DIR, start=BASE_DIR)])
        logging.error(
            "Regression found. To inspect the experiment data for the failed regression test, run\n"
            "sudo ./extract-regression-experiment.sh {test}-{rev}\n"
            "in the ~/infrastructure/hosts/linux-buildbot-worker directory "
            "on the Linux buildbot computer.".format(**locals()))
    exp_dir = get_exp_dir(rev, test)
    eval_dir = exp_dir + "-eval"
    shutil.rmtree(exp_dir)
    shutil.rmtree(eval_dir)
    if not success:
        sys.exit(1)

def main():
    args = parse_custom_args()

    if args.revision.lower() == 'baseline':
        rev = BASELINE
        name = 'baseline'
    else:
        rev = cached_revision.get_global_rev(REPO, rev=args.revision)
        name = rev

    exp = FastDownwardExperiment(path=get_exp_dir(name, args.test), revision_cache=REVISION_CACHE)
    exp.add_suite(BENCHMARKS_DIR, SUITES[args.test])
    for config_nick, config in CONFIGS[args.test]:
        exp.add_algorithm(rev + "-" + config_nick, REPO, rev, config)

    exp.add_parser(exp.EXITCODE_PARSER)
    exp.add_parser(exp.TRANSLATOR_PARSER)
    exp.add_parser(exp.SINGLE_SEARCH_PARSER)
    exp.add_parser(exp.PLANNER_PARSER)

    exp.add_step('build', exp.build)
    exp.add_step('start', exp.start_runs)
    exp.add_fetcher(name='fetch')
    exp.add_report(AbsoluteReport(attributes=ABSOLUTE_ATTRIBUTES), name='report')

    # Only compare results if we are not running the baseline experiment.
    if rev != BASELINE:
        def result_handler(success):
            regression_test_handler(args.test, rev, success)

        exp.add_fetcher(
            src=get_exp_dir('baseline', args.test) + '-eval',
            dest=exp.eval_dir,
            merge=True,
            name='fetch-baseline-results')
        exp.add_report(AbsoluteReport(attributes=ABSOLUTE_ATTRIBUTES), name='comparison')
        exp.add_report(
            RegressionCheckReport(BASELINE, RELATIVE_CHECKS, result_handler),
            name='regression-check')

    exp.run_steps()

main()
