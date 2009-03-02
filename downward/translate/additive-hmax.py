#! /usr/bin/env python2.5
# -*- coding: utf-8 -*-

import pddl
import relaxed_tasks

from collections import defaultdict
from heapq import heappush, heappop


def hmax(task):
    task = relaxed_tasks.build_relaxed_task(task)
    task.convert_to_canonical_form()

    # hmax costs are pairs of the form (cost, depth), where "depth" is
    # the length (as opposed to cost sum) of the supporting chain. We
    # use the depth for tie-breaking to avoid problems with cycles of
    # zero-cost actions.
    infinity = float("inf")

    fact_hmax = {}
    fact_reached_by = {}
    for fact in task.atoms:
        fact_hmax[fact] = (infinity, infinity)
    init, = task.init
    goal, = task.goals
    fact_hmax[init] = (0, 0)
    fact_reached_by[init] = None

    precondition_to_operators = defaultdict(list)
    for action in task.actions:
        preconditions = action.preconditions
        assert preconditions, "relaxed task not in canonical form"
        action.unsatisfied_conditions = len(preconditions)
        for prec in preconditions:
            precondition_to_operators[prec].append(action)

    heap = [((0, 0), init)]
    while heap:
        hmax, fact = heappop(heap)
        if fact == goal:
            break
        if hmax == fact_hmax[fact]:
            for action in precondition_to_operators[fact]:
                action.unsatisfied_conditions -= 1
                if not action.unsatisfied_conditions:
                    hmax_cost, hmax_depth = hmax
                    hmax_cost += action.cost
                    hmax_depth += 1
                    eff_hmax = (hmax_cost, hmax_depth)
                    action.hmax = (hmax_cost, hmax_depth)
                    for effect in action.effects:
                        if eff_hmax < fact_hmax[effect]:
                            fact_hmax[effect] = eff_hmax
                            fact_reached_by[effect] = action
                            heappush(heap, (eff_hmax, effect))
    chain = []
    goal = "@@goal"
    while fact_reached_by[goal]:
        action = fact_reached_by[goal]
        chain.append(action.name)
        goal = max(action.preconditions, key=fact_hmax.__getitem__)
    chain.reverse()
    return hmax, chain


if __name__ == "__main__":
    task = pddl.open()
    # relaxed_task.dump()
    print hmax(task)
