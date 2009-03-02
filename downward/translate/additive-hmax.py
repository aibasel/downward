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
    while heap:
        hmax, fact = heappop(heap)
        if fact == goal:
            # break ## Commented out for now because we want the complete
            #       ## supporters.
            pass
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
    return goal.hmax


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
    # TODO: Unless there's a bug, this should indeed compute a cut
    # (and hence, an action landmark), but it's larger than necessary
    # because it may includes actions that cannot be reached from the
    # init partition of the cut without crossing into the goal partition
    # of the cut first.
    #
    # So we should compute a smaller cut. We should be able to come up
    # with a much smaller cut directly, but if we cannot, there's also
    # the general technique of post-processing the cut until it is
    # guaranteed to be a minimal action landmark.
    goal, = task.goals
    hmax = goal.hmax[0]
    goal_plateau = set()
    def recurse(subgoal):
        if subgoal not in goal_plateau:
            goal_plateau.add(subgoal)
            for action in subgoal.effect_of:
                if action.cost == 0:
                    supporter = action.hmax_supporters[0]
                    recurse(supporter)
    recurse(goal)
    cut = set()
    for action in task.actions:
        if action.hmax_supporters[0] not in goal_plateau:
            for effect in action.effects:
                if effect in goal_plateau:
                    cut.add(action)
                    break
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
