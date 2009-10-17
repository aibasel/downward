#! /usr/bin/env python
# -*- coding: latin-1 -*-

from __future__ import with_statement
from collections import deque
import time

import invariants
import pddl
import timers

class BalanceChecker(object):
  def __init__(self, task):
    self.predicates_to_add_actions = {}
    for action in task.actions:
      for eff in action.effects:
        if not eff.literal.negated:
          predicate = eff.literal.predicate
          self.predicates_to_add_actions.setdefault(predicate, set()).add(action)
  def get_threats(self, predicate):
    return self.predicates_to_add_actions.get(predicate, set())
  def are_compatible(self, add_effect1, add_effect2):
    assert not add_effect1.negated
    assert not add_effect2.negated
    # Check whether two add effects of the same action can happen together.
    # Should always be true for STRIPS actions, but ADL actions can have
    # conditional effects with conflicting triggers.
    # For the invariant finding algorithm to be correct, this may never return
    # "False" unless there really is a conflict. Return "True" although there
    # is no conflict is fine, but can lead to fewer invariants being found.
    return True
  def can_compensate(self, del_effect, add_effect):
    assert del_effect.negated
    assert not add_effect.negated
    # Check whether the del_effect always happens whenever the add_effect
    # happens; they are always effects of the same action.
    # For STRIPS actions, we only need to check whether the del_effect is
    # guaranteed to delete something, i.e. contains a fact mentioned in the
    # precondition. For ADL actions, we should also check that a possible
    # triggering condition always holds whenever the add effect is triggered.
    # For the invariant finding algorithm to be correct, this may never return
    # "True" unless the delete effect is guaranteed to happen when the add
    # effect happens. Returning "False" too often is no problem, but leads to
    # fewer invariants being found.
    # TODO: The current implementation is not correct, but works well enough
    #       in practice. Should perhaps be rectified in the future.
    if add_effect.parameters:
      # Dealing with these in a less conservative ways requires checking that
      # the quantification is *not* over an [omitted] variable, checking that
      # the delete effect is also quantified and unifying the quantified variables
      # in some way. Quite difficult, and besides might need to be done *earlier*
      # than these, because can_compensate might not even be called if the
      # variables in the delete effect are named differently. (This will be the
      # case because of unique variable names.)
      return False
    else:
      return True

def get_fluents(task):
  fluent_names = set()
  for action in task.actions:
    for eff in action.effects:
      fluent_names.add(eff.literal.predicate)
  return [pred for pred in task.predicates if pred.name in fluent_names]

def get_initial_invariants(task):
  for predicate in get_fluents(task):
    all_args = range(len(predicate.arguments))
    for omitted_arg in [-1] + all_args:
      order = [i for i in all_args if i != omitted_arg]
      part = invariants.InvariantPart(predicate.name, order, omitted_arg)
      yield invariants.Invariant((part,))

# Input file might be grounded, beware of too many invariant candidates
MAX_CANDIDATES = 100000
MAX_TIME = 300

def find_invariants(task):
  candidates = deque(get_initial_invariants(task))
  print len(candidates), "initial candidates"
  seen_candidates = set(candidates)

  balance_checker = BalanceChecker(task)

  def enqueue_func(invariant):
    if len(seen_candidates) < MAX_CANDIDATES and invariant not in seen_candidates:
      candidates.append(invariant)
      seen_candidates.add(invariant)

  start_time = time.clock()
  while candidates:
    candidate = candidates.popleft()
    if time.clock() - start_time > MAX_TIME:
      print "Time limit reached, aborting invariant generation"
      return
    if candidate.check_balance(balance_checker, enqueue_func):
      yield candidate

def useful_groups(invariants, initial_facts):
  predicate_to_invariants = {}
  for invariant in invariants:
    for predicate in invariant.predicates:
      predicate_to_invariants.setdefault(predicate, []).append(invariant)

  nonempty_groups = set()
  overcrowded_groups = set()
  for atom in initial_facts:
    if isinstance(atom, pddl.Assign):
      continue
    for invariant in predicate_to_invariants.get(atom.predicate, ()):
      group_key = (invariant, tuple(invariant.get_parameters(atom)))
      if group_key not in nonempty_groups:
        nonempty_groups.add(group_key)
      else:
        overcrowded_groups.add(group_key)
  useful_groups = nonempty_groups - overcrowded_groups
  for (invariant, parameters) in useful_groups:
    yield [part.instantiate(parameters) for part in invariant.parts]

def get_groups(task):
  with timers.timing("Finding invariants"):
    invariants = list(find_invariants(task))
  with timers.timing("Checking invariant weight"):
    result = list(useful_groups(invariants, task.init))
  return result

if __name__ == "__main__":
  import pddl
  print "Parsing..."
  task = pddl.open()
  print "Finding invariants..."
  for invariant in find_invariants(task):
    print invariant
  print "Finding fact groups..."
  groups = get_groups(task)
  for group in groups:
    print "[%s]" % ", ".join(map(str, group))
