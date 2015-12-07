# -*- coding: utf-8 -*-

from __future__ import print_function

import itertools
import os
import os.path
import re


_PLAN_INFO_REGEX = re.compile(r"; cost = (\d+) \((unit cost|general cost)\)\n")


def _read_last_line(filename):
    line = None
    with open(filename) as input_file:
        for line in input_file:
            pass
    return line


def _parse_plan(plan_filename):
    """Parse a plan file and return a pair (cost, problem_type)
    summarizing the salient information. Return (None, None) for
    incomplete plans."""

    last_line = _read_last_line(plan_filename) or ""
    match = _PLAN_INFO_REGEX.match(last_line)
    if match:
        return int(match.group(1)), match.group(2)
    else:
        return None, None


class PlanManager(object):
    def __init__(self, plan_prefix):
        self._plan_prefix = plan_prefix
        self._plan_costs = []
        self._problem_type = None

    def get_plan_prefix(self):
        return self._plan_prefix

    def get_plan_counter(self):
        return len(self._plan_costs)

    def get_best_plan_cost(self):
        """Return best plan cost found so far. Return string
        "infinity" if no plans found yet."""
        if self._plan_costs:
            return self._plan_costs[-1]
        else:
            return "infinity"

    def get_problem_type(self):
        if self._problem_type is None:
            raise ValueError("no plans found yet: cost type not set")
        return self._problem_type

    def process_new_plans(self):
        """Update information about plans after a planner run.

        Read newly generated plans and store the relevant information.
        If the last plan file is incomplete, delete it.
        """

        had_incomplete_plan = False
        for counter in itertools.count(self.get_plan_counter() + 1):
            plan_filename = self._get_plan_file(counter)
            def bogus_plan(msg):
                raise RuntimeError("%s: %s" % (plan_filename, msg))
            if not os.path.exists(plan_filename):
                break
            if had_incomplete_plan:
                bogus_plan("plan found after incomplete plan")
            cost, problem_type = _parse_plan(plan_filename)
            if cost is None:
                had_incomplete_plan = True
                print("%s is incomplete. Deleted the file." % plan_filename)
                os.remove(plan_filename)
            else:
                print("plan manager: found new plan with cost %d" % cost)
                if self._problem_type is None:
                    # This is the first plan we found.
                    self._problem_type = problem_type
                else:
                    # Check if info from this plan matches previous info.
                    if self._problem_type != problem_type:
                        bogus_plan("problem type has changed")
                    if cost >= self._plan_costs[-1]:
                        bogus_plan("plan quality has not improved")
                self._plan_costs.append(cost)

    def get_existing_plans(self):
        """Yield all plans that match the given plan prefix."""
        if os.path.exists(self._plan_prefix):
            yield self._plan_prefix

        for counter in itertools.count(start=1):
            plan_filename = self._get_plan_file(counter)
            if os.path.exists(plan_filename):
                yield plan_filename
            else:
                break

    def delete_existing_plans(self):
        """Delete all plans that match the given plan prefix."""
        for plan in self.get_existing_plans():
            os.remove(plan)

    def _get_plan_file(self, number):
        return "%s.%d" % (self._plan_prefix, number)
