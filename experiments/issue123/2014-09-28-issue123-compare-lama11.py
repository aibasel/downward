#! /usr/bin/env python

from standard_experiment import REMOTE, get_exp

#from lab.reports import Attribute, avg

import os.path

# Set the following variables for the experiment
REPO_NAME = 'fd-issue123'
# suites, e.g. ['gripper:prob01.pddl', 'zenotravel:pfile1'] or suites.suite_satisficing_with_ipc11()
LOCAL_SUITE = []
GRID_SUITE = []
# configs, e.g. '--search', 'astar(lmcut())' for config
CONFIGS = {
    'dummy': ['']
}

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
              configs=CONFIGS)
exp.add_score_attributes()
exp.add_extra_attributes(['quality'])
exp.add_fetcher(os.path.expanduser('~/experiments/executed/issue123/2014-09-28-issue123-base-eval'), filter_config_nick=['lama-2011'])
exp.add_fetcher(os.path.expanduser('~/experiments/executed/issue123/2014-09-28-issue123-v1-eval'), filter_config_nick=['lama-2011-separated'])
REV_BASE = 'issue123-base'
REV_V1 = 'issue123-v1'
configs = [('%s-lama-2011' % REV_BASE, '%s-%s-%s-lama-2011-separated' % (REV_BASE, REV_BASE, REV_V1))]
exp.add_configs_report(compared_configs=configs)

exp()
