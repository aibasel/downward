# -*- coding: utf-8 -*-

from __future__ import print_function

import itertools
import os
import os.path
import re


PLAN_COST_REGEX = re.compile(r"; cost = (\d+) \((unit cost|general cost)\)\n")


def _read_last_line(filename):
    line = None
    with open(filename) as input_file:
        for line in input_file:
            pass
    return line


def _plan_file_is_complete(plan_file):
    last_line = _read_last_line(plan_file) or ""
    return PLAN_COST_REGEX.match(last_line)


def _get_plan_cost_and_cost_type(plan_file):
    last_line = _read_last_line(plan_file)
    match = PLAN_COST_REGEX.match(last_line)
    return int(match.group(1)), match.group(2)


def get_plan_file(plan_prefix, number):
    return "%s.%d" % (plan_prefix, number)


def _get_plan_files_and_clean_up(plan_prefix):
    """Return all complete plan files.

    If the last plan file is incomplete, delete it.
    """
    plan_files = []
    for index in itertools.count(start=1):
        plan_file = get_plan_file(plan_prefix, index)
        if os.path.exists(plan_file):
            if _plan_file_is_complete(plan_file):
                plan_files.append(plan_file)
            else:
                os.remove(plan_file)
                print("%s is incomplete. Deleted the file." % plan_file)
                break
        else:
            break
    return plan_files


def get_cost_type(plan_prefix):
    # This method is only called after a plan has been found.
    plan_files = _get_plan_files_and_clean_up(plan_prefix)
    assert plan_files
    _, cost_type = _get_plan_cost_and_cost_type(plan_files[0])
    return cost_type


def get_g_bound_and_number_of_plans(plan_prefix):
    plan_costs = []
    for plan_file in _get_plan_files_and_clean_up(plan_prefix):
        plan_cost, _ = _get_plan_cost_and_cost_type(plan_file)
        if plan_costs and not plan_costs[-1] > plan_cost:
            raise SystemExit(
                "Plan costs must decrease: %s" %
                " -> ".join(str(c) for c in plan_costs + [plan_cost]))
        plan_costs.append(plan_cost)
    bound = min(plan_costs) if plan_costs else "infinity"
    return bound, len(plan_costs)
