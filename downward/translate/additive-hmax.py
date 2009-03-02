#! /usr/bin/env python2.5
# -*- coding: utf-8 -*-

import pddl
import relaxed_tasks

from heapq import heappush, heappop


def hmax(task):
    task = relaxed_tasks.build_relaxed_task(task)
    task.convert_to_canonical_form()

    # hmax costs are pairs of the form (cost, depth), where "depth" is
    # the length (as opposed to cost sum) of the supporting chain. We
    # use the depth for tie-breaking to avoid problems with cycles of
    # zero-cost actions.
    infinity = float("inf")

    init, = task.init
    goal, = task.goals
    for fact in task.atoms:
        fact.hmax = (infinity, infinity)
        fact.precondition_of = []
    init.hmax = (0, 0)
    init.reached_by = None

    for action in task.actions:
        preconditions = action.preconditions
        assert preconditions, "relaxed task not in canonical form"
        action.unsatisfied_conditions = len(preconditions)
        for prec in preconditions:
            prec.precondition_of.append(action)

    heap = [((0, 0), init)]
    while heap:
        hmax, fact = heappop(heap)
        if fact == goal:
            break
        if hmax == fact.hmax:
            for action in fact.precondition_of:
                action.unsatisfied_conditions -= 1
                if not action.unsatisfied_conditions:
                    hmax_cost, hmax_depth = hmax
                    hmax_cost += action.cost
                    hmax_depth += 1
                    eff_hmax = (hmax_cost, hmax_depth)
                    action.hmax = (hmax_cost, hmax_depth)
                    for effect in action.effects:
                        if eff_hmax < effect.hmax:
                            effect.hmax = eff_hmax
                            effect.reached_by = action
                            heappush(heap, (eff_hmax, effect))
    chain = []
    goal, = task.goals
    while goal.reached_by:
        action = goal.reached_by
        chain.append(action.name)
        goal = max(action.preconditions, key=lambda fact: fact.hmax)
    chain.reverse()
    return hmax, chain


if __name__ == "__main__":
    task = pddl.open()
    # relaxed_task.dump()
    print hmax(task)
