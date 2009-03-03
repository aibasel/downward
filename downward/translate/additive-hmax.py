#! /usr/bin/env python2.5
# -*- coding: utf-8 -*-

import pddl
import relaxed_tasks

from heapq import heappush, heappop


infinity = float("inf")


def hmax(task, goal_cut_facts=()):
    # hmax costs are pairs of the form (cost, depth), where "depth" is
    # the length (as opposed to cost sum) of the supporting chain. We
    # use the depth for tie-breaking to avoid problems with cycles of
    # zero-cost actions.
    init, = task.init
    goal, = task.goals
    for fact in task.atoms:
        fact.hmax = (infinity, infinity)
        fact.precondition_of = []
        fact.effect_of = []
    init.hmax = (0, 0)
    init.reached_by = []

    for action in task.actions:
        preconditions = action.preconditions
        assert preconditions, "relaxed task not in canonical form"
        action.unsatisfied_conditions = len(preconditions)
        for prec in preconditions:
            prec.precondition_of.append(action)
        for eff in action.effects:
            eff.effect_of.append(action)

    heap = [((0, 0), init)]
    cut = []
    while heap:
        hmax, fact = heappop(heap)
        # Note that unlike general hmax computations, we can't break
        # out as soon as we've reached the goal because we also need
        # to compute hmax supporters for all actions.
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
                        if effect in goal_cut_facts:
                            cut.append(action)
                        elif action_hmax < effect.hmax:
                            effect.hmax = action_hmax
                            effect.reached_by = [action]
                            heappush(heap, (action_hmax, effect))
                        elif action_hmax == effect.hmax:
                            effect.reached_by.append(action)
    return goal.hmax[0], cut


def collect_cut(task):
    goal, = task.goals
    goal_plateau = set()
    def recurse(subgoal):
        if subgoal not in goal_plateau:
            goal_plateau.add(subgoal)
            for action in subgoal.effect_of:
                if action.cost == 0:
                    supporter = action.hmax_supporters[0]
                    recurse(supporter)
    recurse(goal)
    cut_hmax, cut = hmax(task, goal_plateau)
    assert cut_hmax == infinity, "did not extract a landmark!"
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
