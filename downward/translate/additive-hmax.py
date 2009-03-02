#! /usr/bin/env python2.5
# -*- coding: utf-8 -*-

import pddl
import relaxed_tasks

from heapq import heappush, heappop


def hmax(task):
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
    init.reached_by = []

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
                    action_hmax = (hmax_cost, hmax_depth)
                    action.hmax = action_hmax
                    action.hmax_supporters = [
                        fact for fact in action.preconditions
                        if fact.hmax == hmax]
                    for effect in action.effects:
                        if action_hmax < effect.hmax:
                            effect.hmax = action_hmax
                            effect.reached_by = [action]
                            heappush(heap, (action_hmax, effect))
                        elif action_hmax == effect.hmax:
                            effect.reached_by.append(action)
    # chain = collect_chain(task)
    return hmax


def collect_chain(task):
    # NOTE: We backchain by taking the first best supporter of each
    # action and fact. All other choices would also be fine, and might
    # lead to different landmarks being extracted.
    chain = []
    goal, = task.goals
    while goal.reached_by:
        action = goal.reached_by[0]
        chain.append(action.name)
        goal = action.hmax_supporters[0]
    chain.reverse()
    return chain


def collect_cut(task):
    # BUG: Unfortunately, this is buggy -- by only considering
    # "reached_by", which are the *best* supporters of each fact,
    # we're not guaranteed to get a cut of the justification graph
    # here.
    goal, = task.goals
    hmax = goal.hmax[0]
    cut = set()
    def recurse(supporter):
        assert supporter.hmax[0] == hmax
        for action in supporter.reached_by:
            # NOTE: Same as in collect_chain: could just as well pick
            # another supporter here instead of hmax_supporters[0]
            supporter = action.hmax_supporters[0]
            supporter_hmax = supporter.hmax[0]
            assert supporter_hmax <= hmax
            if supporter_hmax < hmax:
                cut.add(action)
            else:
                recurse(supporter)
    recurse(goal)
    return cut


def additive_hmax(task):
    MULTIPLIER = 1
    result = 0
    for action in task.actions:
        action.cost *= MULTIPLIER
    while True:
        cost, _ = hmax(task)
        if cost == 0:
            break
        result += 1
        cut = collect_cut(task)
        for action in cut:
            assert action.cost >= 1
            action.cost -= 1
    return (result + MULTIPLIER - 1) // MULTIPLIER


if __name__ == "__main__":
    task = pddl.open()
    relaxed_task = relaxed_tasks.build_relaxed_task(task)
    relaxed_task.convert_to_canonical_form()
    # relaxed_task.dump()
    print additive_hmax(relaxed_task)
