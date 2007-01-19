#! /usr/bin/env python2.4
# -*- coding: latin-1 -*-

import axiom_rules
import instantiate
import invariant_finder
import pddl
import sas_tasks

# TODO: The translator may generate trivial derived variables which are always true,
# for example if there ia a derived predicate in the input that only depends on
# (non-derived) variables which are detected as always true.
# Such a situation was encountered in the PSR-STRIPS-DerivedPredicates domain.
# Such "always-true" variables should best be compiled away, but it is
# not clear what the best place to do this should be. Similar
# simplifications might be possible elsewhere, for example if a
# derived variable is synonymous with another variable (derived or
# non-derived).

ALLOW_CONFLICTING_EFFECTS = False

def tuples(domain, n):
  if n == 0:
    yield ()
  else:
    for tup in tuples(domain, n - 1):
      for obj in domain:
        yield tup + (obj,)

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

def strips_to_sas_dictionary(groups):
  dictionary = {}
  for var_no, group in enumerate(groups):
    for val_no, atom in enumerate(group):
      dictionary[atom] = (var_no, val_no)
  return [len(group) + 1 for group in groups], dictionary

def translate_strips_conditions(conditions, dictionary, ranges):
  if not conditions:
    return {} # Quick exit for common case.

  condition = {}
  for fact in conditions:
    if fact.negated:
      ## BUG: Here we take a shortcut compared to Sec. 10.6.4
      ##      of the thesis and do something that doesn't appear
      ##      to make sense if this is part of a proper fact group.
      ##      Compare the last sentences of the third paragraph of
      ##      the section.
      ##      We need to do what is written there. As a test case,
      ##      consider Airport ADL tasks with only one airport, where
      ##      (occupied ?x) variables are encoded in a single variable,
      ##      and conditions like (not (occupied ?x)) do occur in
      ##      preconditions.
      fact = pddl.Atom(fact.predicate, fact.args)
      var, _ = dictionary[fact]
      val = ranges[var] - 1
    else:
      var, val = dictionary[fact]
    if condition.get(var) not in (None, val):
      # Conflicting conditions on this variable: Operator invalid.
      return None
    condition[var] = val
  return condition

# Old version had problem with
# - delete effects
# - without matching precondition or effect condition
# - without corresponding add effect
# - with range > 2.
# These need to be conditional effects.
# "If set, then delete."

def translate_strips_operator(operator, dictionary, ranges):
  # NOTE: This function does not really deal with the intricacies of properly
  # encoding delete effects for grouped propositions in the presence of
  # conditional effects. It should work ok but will bail out in more
  # complicated cases even though a conflict does not necessarily exist.

  possible_add_conflict = False
  
  condition = translate_strips_conditions(operator.precondition, dictionary, ranges)
  if condition is None:
    return None

  effect = {}

  for conditions, fact in operator.add_effects:
    eff_condition = translate_strips_conditions(conditions, dictionary, ranges)
    if eff_condition is None: # Impossible condition for this effect.
      continue
    eff_condition = eff_condition.items()
    var, val = dictionary[fact]
    effect_pair = effect.get(var)
    if not effect_pair:
      effect[var] = (val, [eff_condition])
    else:
      other_val, eff_conditions = effect_pair
      # Don't flag conflict just yet... the operator might be invalid
      # because of conflicting add/delete effects (see pipesworld).
      if other_val != val:
        possible_add_conflict = True
      eff_conditions.append(eff_condition)

  for conditions, fact in operator.del_effects:
    eff_condition_dict = translate_strips_conditions(conditions, dictionary, ranges)
    if eff_condition_dict is None:
      continue
    eff_condition = eff_condition_dict.items()
    var, val = dictionary[fact]
    none_of_those = ranges[var] - 1

    other_val, eff_conditions = effect.setdefault(var, (none_of_those, []))

    if other_val != none_of_those:
      # Look for matching add effect; ignore this del effect if found.
      assert eff_condition in eff_conditions or [] in eff_conditions, \
             "Add effect with uncertain del effect partner?"
      if other_val == val:
        if ALLOW_CONFLICTING_EFFECTS:
          # Conflicting ADD and DEL effect. This is *only* allowed if
          # this is also a precondition, in which case there is *no*
          # effect (the ADD takes precedence). We delete the add effect here.
          assert condition.get(var) == val
          assert eff_conditions == [[]]
          del effect[var]
        else:
          assert not eff_condition and not eff_conditions[0], "Uncertain conflict"
          return None  # Definite conflict otherwise.
    else:
      if condition.get(var) != val and eff_condition_dict.get(var) != val:
        # Need a guard for this delete effect.
        assert var not in condition and var not in eff_condition, "Oops?"
        eff_condition.append((var, val))
      eff_conditions.append(eff_condition)

  if possible_add_conflict:
    print operator.name

  assert not possible_add_conflict, "Conflicting add effects?"

  # assert eff_condition != other_condition, "Duplicate effect"
  # assert eff_condition and other_condition, "Dominated conditional effect"

  pre_post = []
  for var, (post, eff_condition_lists) in effect.iteritems():
    pre = condition.get(var, -1)
    if pre != -1:
      del condition[var]
    for eff_condition in eff_condition_lists:
      pre_post.append((var, pre, post, eff_condition))
  prevail = condition.items()

  return sas_tasks.SASOperator(operator.name, prevail, pre_post)

"""
def translate_strips_operator(operator, dictionary, ranges):
  # NOTE: This function does not really deal with the intricacies of properly
  # encoding delete effects for grouped propositions in the presence of
  # conditional effects. It probably works but will break with more
  # complicated conditional effects.
  
  condition = translate_strips_conditions(operator.precondition, dictionary, ranges)
  if condition is None:
    return None

  effect = {}

  for conditions, fact in operator.add_effects:
    eff_condition = translate_strips_conditions(conditions, dictionary, ranges).items()
    if eff_condition is None: # Impossible condition for this effect.
      continue
    var, val = dictionary[fact]
    effect_list = effect.setdefault(var, [])
    if effect_list:
      # There are previous effects that affect the same variable. In the
      # presence of conditional effects, this is ok, but there might be
      # something fishy with the invariants, so we are rather safe than sorry.
      # This can be removed in the final version to improve speed.
      for other_val, other_cond in effect_list:
        if other_val == val:
          assert eff_condition != other_condition, "Duplicate effect"
          assert eff_condition and other_condition, "Dominated conditional effect"
        else:
          # This check is too conservative.
          assert eff_condition and other_condition, "Conflicting effects?"
    effect_list.append((val, eff_condition))

  for conditions, fact in operator.del_effects:
    eff_condition = translate_strips_conditions(conditions, dictionary, ranges).items()
    if eff_condition is None:
      continue
    var, val = dictionary[fact]
    effect_list = effect.setdefault(var, [])
    none_of_those = ranges[var] - 1
    if effect_list:
      for other_val, other_cond in effect_list:
        if other_val != none_of_those:
          # There is a corresponding add effect.
          # Check that it *really* matches and don't do anything.
          # The add effect sets the variable already.
          assert other_cond == eff_condition, "Operator too complicated"
          if not ALLOW_CONFLICTING_EFFECTS and other_val == val:
            # We are adding and deleting the same value, with the same
            # condition. This might or might not be illegal, depending on
            # which semantics for PDDL you adhere to.
            return None
        else:
          # There is another delete effect.
          # Do the same check as for add effects above, then
          # add a different conditional delete effect if appropriate.
          assert eff_condition != other_condition, "Duplicate effect"
          assert eff_condition and other_condition, "Dominated conditional effect"
          effect_list.append((none_of_those, eff_condition))
    else:
      effect_list.append((none_of_those, eff_condition))

  pre_post = []
  for var, eff_list in effect.iteritems():
    pre = condition.get(var, -1)
    if pre != -1:
      del condition[var]
    for (post, eff_condition) in eff_list:
      pre_post.append((var, pre, post, eff_condition))
  prevail = condition.items()

  return sas_tasks.SASOperator(operator.name, prevail, pre_post)
"""

def translate_strips_axiom(axiom, dictionary, ranges):
  condition = translate_strips_conditions(axiom.condition, dictionary, ranges)
  if condition is None:
    return None
  if axiom.effect.negated:
    var, _ = dictionary[axiom.effect.positive()]
    effect = (var, ranges[var] - 1)
  else:
    effect = dictionary[axiom.effect]
  return sas_tasks.SASAxiom(condition.items(), effect)

def translate_strips_operators(actions, strips_to_sas, ranges):
  result = []
  for action in actions:
    sas_op = translate_strips_operator(action, strips_to_sas, ranges)
    if sas_op:
      result.append(sas_op)
  return result

def translate_strips_axioms(axioms, strips_to_sas, ranges):
  result = []
  for axiom in axioms:
    sas_axiom = translate_strips_axiom(axiom, strips_to_sas, ranges)
    if sas_axiom:
      result.append(sas_axiom)
  return result

def translate_task(strips_to_sas, ranges, init, goals, actions, axioms,
                   axiom_layer_dict):
  axiom_layers = [-1] * len(ranges)
  for atom, layer in axiom_layer_dict.iteritems():
    assert layer >= 0
    axiom_layers[strips_to_sas[atom][0]] = layer
  variables = sas_tasks.SASVariables(ranges, axiom_layers)

  init_values = [rang - 1 for rang in ranges]
  # Closed World Assumption: Initialize to "range - 1" == Nothing.
  for fact in init:
    pair = strips_to_sas.get(fact)
    if pair:
      var, val = pair
      assert init_values[var] == ranges[var] - 1
      init_values[var] = val
  init = sas_tasks.SASInit(init_values)

  goal_pairs = translate_strips_conditions(goals, strips_to_sas, ranges).items()
  goal = sas_tasks.SASGoal(goal_pairs)

  operators = translate_strips_operators(actions, strips_to_sas, ranges)
  axioms = translate_strips_axioms(axioms, strips_to_sas, ranges)

  return sas_tasks.SASTask(variables, init, goal, operators, axioms)

def unsolvable_sas_task():
  print "No relaxed solution! Generating unsolvable task..."
  variables = sas_tasks.SASVariables([2], [-1])
  init = sas_tasks.SASInit([0])
  goal = sas_tasks.SASGoal([(0, 1)])
  operators = []
  axioms = []
  return sas_tasks.SASTask(variables, init, goal, operators, axioms)

def pddl_to_sas(task):
  print "Instantiating..."
  relaxed_reachable, atoms, actions, axioms = instantiate.explore(task)

  if not relaxed_reachable:
    return unsolvable_sas_task()

  # HACK! Goals should be treated differently (see TODO file).
  if isinstance(task.goal, pddl.Conjunction):
    goal_list = task.goal.parts
  else:
    assert isinstance(task.goal, pddl.Literal)
    goal_list = [task.goal]

  axioms, axiom_init, axiom_layers = axiom_rules.handle_axioms(atoms, actions,
                                                               axioms, goal_list)
  #axioms.sort(key=lambda axiom: axiom.name)
  #for axiom in axioms:
  #  axiom.dump()

  print "Finding invariants..."
  groups = invariant_finder.get_groups(task)
  print "Instantiating groups..."
  groups = instantiate_groups(groups, task, atoms)
  print "Choosing groups..."
  groups = choose_groups(groups, atoms)

  groups_file = file("test.groups", "w")
  for i, group in enumerate(groups):
    print >> groups_file, "var%d:" % i
    for j, fact in enumerate(group):
      print >> groups_file, "  %d: %s" % (j, fact)
    print >> groups_file, "  %d: <none of those>" % len(group)
  groups_file.close()

  print "Building STRIPS to SAS dictionary..."
  ranges, strips_to_sas = strips_to_sas_dictionary(groups)
  print "Translating task..."

  sas_task = translate_task(strips_to_sas, ranges, task.init + axiom_init, goal_list,
                            actions, axioms, axiom_layers)
  return sas_task

if __name__ == "__main__":
  import pddl
  print "Parsing..."
  task = pddl.open()
  if task.domain_name == "protocol":
    ALLOW_CONFLICTING_EFFECTS = True

  # EXPERIMENTAL!
  # import psyco
  # psyco.full()

  sas_task = pddl_to_sas(task)
  print "Writing output..."
  sas_task.output(file("output.sas", "w"))
  print "Done!"
