#! /usr/bin/env python

from standard_experiment import REMOTE, get_exp

from downward import suites
#from lab.reports import Attribute, avg

import os.path

# Set the following variables for the experiment
REPO_NAME = 'fd-issue123'
# revisions, e.g. ['3d6c1ccacdce']
REVISIONS = ['issue123-base']
# suites, e.g. ['gripper:prob01.pddl', 'zenotravel:pfile1'] or suites.suite_satisficing_with_ipc11()
LOCAL_SUITE = ['depot:pfile1']
GRID_SUITE = suites.suite_satisficing_with_ipc11()
# configs, e.g. '--search', 'astar(lmcut())' for config
CONFIGS = {

    'lama-2011': [
        "--if-unit-cost",
        "--heuristic",
        "hlm,hff=lm_ff_syn(lm_rhw(reasonable_orders=true))",
        "--search", "iterated(["
        "                 lazy_greedy([hff,hlm],preferred=[hff,hlm]),"
        "                 lazy_wastar([hff,hlm],preferred=[hff,hlm],w=5),"
        "                 lazy_wastar([hff,hlm],preferred=[hff,hlm],w=3),"
        "                 lazy_wastar([hff,hlm],preferred=[hff,hlm],w=2),"
        "                 lazy_wastar([hff,hlm],preferred=[hff,hlm],w=1)"
        "                 ],repeat_last=true,continue_on_fail=true)",
        "--if-non-unit-cost",
        "--heuristic",
        "hlm1,hff1=lm_ff_syn(lm_rhw(reasonable_orders=true,"
        "                           lm_cost_type=one,cost_type=one))",
        "--heuristic",
        "hlm2,hff2=lm_ff_syn(lm_rhw(reasonable_orders=true,"
        "                           lm_cost_type=plusone,cost_type=plusone))",
        "--search", "iterated(["
        "                 lazy_greedy([hff1,hlm1],preferred=[hff1,hlm1],"
        "                             cost_type=one,reopen_closed=false),"
        "                 lazy_greedy([hff2,hlm2],preferred=[hff2,hlm2],"
        "                             reopen_closed=false),"
        "                 lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=5),"
        "                 lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=3),"
        "                 lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=2),"
        "                 lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=1)"
        "                 ],repeat_last=true,continue_on_fail=true)",
    ],

    'lama-2011-first-it': [
        "--heuristic",
        "hlm,hff=lm_ff_syn(lm_rhw(reasonable_orders=true,"
        "                         lm_cost_type=one,cost_type=one))",
        "--search",
        "lazy_greedy([hff,hlm],preferred=[hff,hlm],cost_type=one)"
    ],

    'lama-2011-separated': [
        "--if-unit-cost",
        "--heuristic",
        "hlm=lmcount(lm_rhw(reasonable_orders=true),pref=true)",
        "--heuristic",
        "hff=ff()",
        "--search", "iterated(["
        "                 lazy_greedy([hff,hlm],preferred=[hff,hlm]),"
        "                 lazy_wastar([hff,hlm],preferred=[hff,hlm],w=5),"
        "                 lazy_wastar([hff,hlm],preferred=[hff,hlm],w=3),"
        "                 lazy_wastar([hff,hlm],preferred=[hff,hlm],w=2),"
        "                 lazy_wastar([hff,hlm],preferred=[hff,hlm],w=1)"
        "                 ],repeat_last=true,continue_on_fail=true)",
        "--if-non-unit-cost",
        "--heuristic",
        "hlm1=lmcount(lm_rhw(reasonable_orders=true,"
        "                    lm_cost_type=one,cost_type=one),"
        "             pref=true,cost_type=one)",
        "--heuristic",
        "hff1=ff(cost_type=one)",
        "--heuristic",
        "hlm2=lmcount(lm_rhw(reasonable_orders=true,"
        "                    lm_cost_type=plusone,cost_type=plusone),"
        "             pref=true,cost_type=plusone)",
        "--heuristic",
        "hff2=ff(cost_type=plusone)",
        "--search", "iterated(["
        "                 lazy_greedy([hff1,hlm1],preferred=[hff1,hlm1],"
        "                             cost_type=one,reopen_closed=false),"
        "                 lazy_greedy([hff2,hlm2],preferred=[hff2,hlm2],"
        "                             reopen_closed=false),"
        "                 lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=5),"
        "                 lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=3),"
        "                 lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=2),"
        "                 lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=1)"
        "                 ],repeat_last=true,continue_on_fail=true)",
    ],

    'lama-2011-first-it-separated': [
        "--heuristic",
        "hlm=lmcount(lm_rhw(reasonable_orders=true,"
        "                    lm_cost_type=one,cost_type=one),"
        "             pref=true,cost_type=one)",
        "--heuristic",
        "hff=ff(cost_type=one)",
        "--search",
        "lazy_greedy([hff,hlm],preferred=[hff,hlm],cost_type=one)",
    ],
}

# limits, e.g. { 'search_time': 120 }
LIMITS = None
# for 'make debug', set to True.
COMPILATION_OPTION = None #(default: 'release')
# choose any lower priority if whished
PRIORITY = None #(default: 0)


# Do not change anything below here
SCRIPT_PATH = os.path.abspath(__file__)
if REMOTE:
    SUITE = GRID_SUITE
    REPO = os.path.expanduser('~/repos/' + REPO_NAME)
else:
    SUITE = LOCAL_SUITE
    REPO = os.path.expanduser('~/work/' + REPO_NAME)


# Create the experiment. Add parsers, fetchers or reports...
exp = get_exp(script_path=SCRIPT_PATH, repo=REPO, suite=SUITE,
              configs=CONFIGS, revisions=REVISIONS, limits=LIMITS,
              compilation_option=COMPILATION_OPTION, priority=PRIORITY)
exp.add_score_attributes()
exp.add_extra_attributes(['quality'])

REV = REVISIONS[0]
configs_lama = [('%s-lama-2011' % REV, '%s-lama-2011-separated' % REV)]
exp.add_configs_report(compared_configs=configs_lama, name='lama')
configs_lama_first_it = [('%s-lama-2011-first-it' % REV, '%s-lama-2011-first-it-separated' % REV)]
exp.add_configs_report(compared_configs=configs_lama_first_it, name='lama-first-it')
exp.add_absolute_report()

exp()
