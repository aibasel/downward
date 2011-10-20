#! /usr/bin/env python
# -*- coding: utf-8 -*-

import pddl
import random
import relaxed_tasks

from heapq import heappush, heappop

random.seed(2009)


infinity = float("inf")


def crossreference_task(task):
    for fact in task.atoms:
        fact.precondition_of = []
        fact.effect_of = []
    for action in task.actions:
        for prec in action.preconditions:
            prec.precondition_of.append(action)
        for eff in action.effects:
            eff.effect_of.append(action)


def hmax(task, goal_cut_facts=()):
    # hmax costs are pairs of the form (cost, depth), where "depth" is
    # the length (as opposed to cost sum) of the supporting chain. We
    # use the depth for tie-breaking; this is not strictly necessary,
    # but empirically leads to better action landmarks (i.e., higher
    # heuristic values).
    init, = task.init
    goal, = task.goals
    for fact in task.atoms:
        fact.hmax = (infinity, infinity)
    init.hmax = (0, 0)

    for action in task.actions:
        assert action.preconditions, "relaxed task not in canonical form"
        action.unsatisfied_conditions = len(action.preconditions)

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

                    # We have some freedom in which hmax_supporter to pick.
                    # Quick summary of results for the following three options:
                    # - Option 2 and 4 give identical results.
                    # - Option 1 vs. Option 2:
                    #   * wins for Option 1: Satellite (+1 twice, +2 once)
                    #   * wins for Option 2: Blocks (+1 once), Logistics98 (+1 once),
                    #     Satellite (+1 twice, +2 once, +3 once, +4 once)
                    # - Option 1 vs. Option 3:
                    #   * wins for Option 1: Gripper (+1 thirteen times),
                    #     Satellite (+1 three times, +2 once)
                    #   * wins for Option 3: Blocks (+1 once), Logistics98 (+1 once),
                    #     Satellite (+1 twice, +2 three times, +3 once)
                    # - Option 2 vs. Option 3:
                    #   * wins for Option 2: Gripper (+1 thirteen times),
                    #     Satellite (+1 once, +2 three times, +3 once)
                    #   * wins for Option 3: Satellite (+1 three times, +2 three times)
                    #
                    # For options 2 and 3, I also made some experiments how
                    # results change when multiplying costs by 5 (i.e., only extracting
                    # a fraction of the action cost per round).
                    # - Option 2:
                    #   * Satellite: +1 (three times), +2 (once), +4 (once)
                    # - Option 3:
                    #   * Gripper: +1 (thirteen times), -1 (once)
                    #   * Satellite: +4 (once), +3 (three times), +2 (one time),
                    #     +1 (five times), - 1 (once)
                    OPTION = 2
                    if OPTION == 1:
                        # Choose first (in precondition list)
                        # maximizing precondition as supporter.
                        supporter = (fact for fact in action.preconditions
                                     if fact.hmax == hmax).next()
                    elif OPTION == 2:
                        # Choose this fact as supporter.
                        supporter = fact
                    elif OPTION == 3:
                        # Choose a random maximizer as a supporter.
                        supporter = random.choice([
                            fact for fact in action.preconditions
                            if fact.hmax == hmax])
                    elif OPTION == 4:
                        # Choose last (in precondition list)
                        # maximizing precondition as supporter.
                        supporter = [fact for fact in action.preconditions
                                     if fact.hmax == hmax][-1]
                    else:
                        assert False
                    action.hmax_supporter = supporter
                    for effect in action.effects:
                        if effect in goal_cut_facts:
                            cut.append(action)
                            break
                        elif action_hmax < effect.hmax:
                            effect.hmax = action_hmax
                            heappush(heap, (action_hmax, effect))
    return goal.hmax[0], cut


def collect_cut(task):
    goal, = task.goals
    goal_plateau = set()
    def recurse(subgoal):
        if subgoal not in goal_plateau:
            goal_plateau.add(subgoal)
            for action in subgoal.effect_of:
                if action.cost == 0:
                    recurse(action.hmax_supporter)
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
    crossreference_task(relaxed_task)
    # relaxed_task.dump()
    print additive_hmax(relaxed_task)
