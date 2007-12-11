# -*- coding: latin-1 -*-

import invariant_finder
import pddl

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
    def __init__(self, groups):
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

def choose_groups(groups, reachable_facts):
    queue = GroupCoverQueue(groups)
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

def compute_groups(task, atoms):
    print "Finding invariants..."
    groups = invariant_finder.get_groups(task)
    print "Instantiating groups..."
    groups = instantiate_groups(groups, task, atoms)
    print "Choosing groups..."
    groups = choose_groups(groups, atoms)
    print "Building translation key..."
    translation_key = build_translation_key(groups)
    return groups, translation_key
