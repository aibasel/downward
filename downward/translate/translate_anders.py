#! /usr/bin/env python2.4
# -*- coding: latin-1 -*-

import axiom_rules
import instantiate
import invariant_finder
import pddl

# TODO: The translator may generate trivial derived variables which are always true,
# for example if there ia a derived predicate in the input that only depends on
# (non-derived) variables which are detected as always true.
# Such a situation was encountered in the PSR-STRIPS-DerivedPredicates domain.
# Such "always-true" variables should best be compiled away, but it is
# not clear what the best place to do this should be. Similar
# simplifications might be possible elsewhere, for example if a
# derived variable is synonymous with another variable (derived or
# non-derived).

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

def add_uncovered_groups(groups, reachable_facts):
  result = list(groups)
  uncovered_facts = reachable_facts.copy()
  for group in groups:
    uncovered_facts.difference_update(group)
  print len(uncovered_facts), "uncovered facts"
  result += [[fact] for fact in uncovered_facts]
  return result

def dump_task(init, goals, actions, axioms, axiom_layer_dict):
  print "Initial state"
  for atom in init:
    print atom
  print
  print "Goals"
  for goal in goals:
    print goal
  for action in actions:
    print
    print "Action"
    action.dump()
  for axiom in axioms:
    print
    print "Axiom"
    axiom.dump()
  print
  print "Axiom layers"
  for atom, layer in axiom_layer_dict.iteritems():
    print "%s: layer %d" % (atom, layer)

def process_task(task):
  print "Instantiating..."
  relaxed_reachable, atoms, actions, axioms = instantiate.explore(task)

  if not relaxed_reachable:
    print "No relaxed solution! Task is unsolvable."
    return

  # HACK! Goals should be treated differently (see TODO file).
  if isinstance(task.goal, pddl.Conjunction):
    goal_list = task.goal.parts
  else:
    assert isinstance(task.goal, pddl.Literal)
    goal_list = [task.goal]

  axioms, axiom_init, axiom_layers = axiom_rules.handle_axioms(atoms, actions,
                                                               axioms, goal_list)
  print "Finding invariants..."
  groups = invariant_finder.get_groups(task)
  print "Instantiating groups..."
  groups = instantiate_groups(groups, task, atoms)
  print "Adding groups for uncovered facts..."
  groups = add_uncovered_groups(groups, atoms)

  groups_file = file("test.groups", "w")
  for i, group in enumerate(groups):
    print >> groups_file, "var%d:" % i
    for j, fact in enumerate(group):
      print >> groups_file, "  %d: %s" % (j, fact)
    print >> groups_file, "  %d: <none of those>" % len(group)
  groups_file.close()

  print "Generating output..."
  relevant_atoms = set(atoms)
  init = ([atom for atom in task.init if atom in relevant_atoms]
          + axiom_init)
  print
  dump_task(init, goal_list, actions, axioms, axiom_layers)

if __name__ == "__main__":
  import pddl
  print "Parsing..."
  task = pddl.open()
  process_task(task)
