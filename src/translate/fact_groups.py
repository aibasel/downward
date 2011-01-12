# -*- coding: latin-1 -*-

from __future__ import with_statement
import itertools

import invariant_finder
import pddl
import timers

def expand_group(group, task, reachable_facts):
    result = []
    for fact in group:
        try:
            pos = list(fact.args).index("?X")
        except ValueError:
            result.append(fact)
        else:
            # NOTE: This could be optimized by only trying objects of the correct
            #       type, or by using a unifier which directly generates the
            #       applicable objects. It is not worth optimizing this at this stage,
            #       though.
            for obj in task.objects:
                newargs = list(fact.args)
                newargs[pos] = obj.name
                atom = pddl.Atom(fact.predicate, newargs)
                if atom in reachable_facts:
                    result.append(atom)
    return result

def instantiate_groups(groups, task, reachable_facts):
    return [expand_group(group, task, reachable_facts) for group in groups]

class GroupCoverQueue:
    def __init__(self, groups, partial_encoding):
        self.partial_encoding = partial_encoding
        if groups:
            self.max_size = max([len(group) for group in groups])
            self.groups_by_size = [[] for i in range(self.max_size + 1)]
            self.groups_by_fact = {}
            for group in groups:
                group = set(group) # Copy group, as it will be modified.
                self.groups_by_size[len(group)].append(group)
                for fact in group:
                    self.groups_by_fact.setdefault(fact, []).append(group)
            self._update_top()
        else:
            self.max_size = 0
    def __nonzero__(self):
        return self.max_size > 1
    def pop(self):
        result = list(self.top) # Copy; this group will shrink further.
        if self.partial_encoding:
            for fact in result:
                for group in self.groups_by_fact[fact]:
                    group.remove(fact)
        self._update_top()
        return result
    def _update_top(self):
        while self.max_size > 1:
            max_list = self.groups_by_size[self.max_size]
            while max_list:
                candidate = max_list.pop()
                if len(candidate) == self.max_size:
                    self.top = candidate
                    return
                self.groups_by_size[len(candidate)].append(candidate)
            self.max_size -= 1

def choose_groups(groups, reachable_facts, partial_encoding=True):
    queue = GroupCoverQueue(groups, partial_encoding=partial_encoding)
    uncovered_facts = reachable_facts.copy()
    result = []
    while queue:
        group = queue.pop()
        uncovered_facts.difference_update(group)
        result.append(group)
    print len(uncovered_facts), "uncovered facts"
    #for fact in uncovered_facts:
    #  print fact
    result += [[fact] for fact in uncovered_facts]
    return result

def build_translation_key(groups):
    group_keys = []
    for group in groups:
        group_key = [str(fact) for fact in group]
        group_key.append("<none of those>")
        group_keys.append(group_key)
    return group_keys

def collect_all_mutex_groups(groups, atoms):
    # NOTE: This should be functionally identical to choose_groups
    # when partial_encoding is set to False. Maybe a future
    # refactoring could take that into account.
    all_groups = []
    uncovered_facts = atoms.copy()
    for group in groups:
        uncovered_facts.difference_update(group)
        all_groups.append(group)
    all_groups += [[fact] for fact in uncovered_facts]
    return all_groups

def add_inequality_preconditions(task, reachable_action_params):
    modified_task = task.copy()
    new_actions = []
    for action in modified_task.actions:
        inequal_params = []
        if len(action.parameters) < 2:
            continue
        combs = itertools.combinations(range(len(action.parameters)), 2)
        for pos1, pos2 in combs:
            inequality = True
            for params in reachable_action_params[action.name]:
                if params[pos1] == params[pos2]:
                    inequality = False
                    break
            if inequality:
                inequal_params.append((pos1, pos2))
        new_action = action.copy()
        new_precond_parts = list(action.precondition.parts)
        for pos1, pos2 in inequal_params:
            param1 = action.parameters[pos1].name
            param2 = action.parameters[pos2].name
            new_cond = pddl.NegatedAtom("=", (param1, param2))
            new_precond_parts.append(new_cond)
        precond = new_action.precondition.change_parts(new_precond_parts)
        new_action.precondition = precond
        new_actions.append(new_action)

    modified_task.actions = new_actions
    return modified_task

def compute_groups(task, atoms, reachable_action_params, partial_encoding=True):
    with timers.timing("Adding inequality preconditions"):
        new_task = add_inequality_preconditions(task, reachable_action_params) 
    groups = invariant_finder.get_groups(new_task)
    with timers.timing("Instantiating groups"):
        groups = instantiate_groups(groups, task, atoms)

    # TODO: I think that collect_all_mutex_groups should do the same thing
    #       as choose_groups with partial_encoding=False, so these two should
    #       be unified.
    with timers.timing("Collecting mutex groups"):
        mutex_groups = collect_all_mutex_groups(groups, atoms)
    with timers.timing("Choosing groups", block=True):
        groups = choose_groups(groups, atoms, partial_encoding=partial_encoding)
    with timers.timing("Building translation key"):
        translation_key = build_translation_key(groups)
    return groups, mutex_groups, translation_key
