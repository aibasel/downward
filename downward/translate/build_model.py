#! /usr/bin/env python2.4
# -*- coding: latin-1 -*-

import sys

import pddl

def convert_rules(prog):
  RULE_TYPES = {"join": JoinRule, "product": ProductRule, "project": ProjectRule}
  result = []
  for rule in prog.rules:
    RuleType = RULE_TYPES[rule.type]
    new_effect, new_conditions = variables_to_numbers(rule.effect, rule.conditions)
    rule = RuleType(new_effect, new_conditions)
    rule.validate()
    result.append(rule)
  return result

def variables_to_numbers(effect, conditions):
  new_effect_args = list(effect.args)
  rename_map = {}
  for i, arg in enumerate(effect.args):
    if arg[0] == "?":
      rename_map[arg] = i
      new_effect_args[i] = i
  new_effect = pddl.Atom(effect.predicate, new_effect_args)

  new_conditions = []
  for cond in conditions:
    new_cond_args = [rename_map.get(arg, arg) for arg in cond.args]
    new_conditions.append(pddl.Atom(cond.predicate, new_cond_args))
  return new_effect, new_conditions

class BuildRule:
  def prepare_effect(self, new_atom, cond_index):
    effect_args = list(self.effect.args)
    cond = self.conditions[cond_index]
    for var_no, obj in zip(cond.args, new_atom.args):
      if isinstance(var_no, int):
        effect_args[var_no] = obj
    return effect_args
  def __str__(self):
    return "%s :- %s" % (self.effect, ", ".join(map(str, self.conditions)))
    
class JoinRule(BuildRule):
  def __init__(self, effect, conditions):
    self.effect = effect
    self.conditions = conditions
    left_args = conditions[0].args
    right_args = conditions[1].args
    left_vars = set([var for var in left_args if isinstance(var, int)])
    right_vars = set([var for var in right_args if isinstance(var, int)])
    common_vars = left_vars & right_vars
    self.common_var_positions = [[args.index(var) for var in common_vars]
                                 for args in (list(left_args), list(right_args))]
    self.atoms_by_key = ({}, {})
  def validate(self):
    assert len(self.conditions) == 2, self
    left_args = self.conditions[0].args
    right_args = self.conditions[1].args
    eff_args = self.effect.args
    left_vars = set([v for v in left_args if isinstance(v, int) or v[0] == "?"])
    right_vars = set([v for v in right_args if isinstance(v, int) or v[0] == "?"])
    eff_vars = set([v for v in eff_args if isinstance(v, int) or v[0] == "?"])
    assert left_vars & right_vars, self
    assert (left_vars | right_vars) == (left_vars & right_vars) | eff_vars
  def update_index(self, new_atom, cond_index):
    ordered_common_args = [new_atom.args[position]
                           for position in self.common_var_positions[cond_index]]
    key = tuple(ordered_common_args)
    self.atoms_by_key[cond_index].setdefault(key, []).append(new_atom)
  def fire(self, new_atom, cond_index, enqueue_func):
    effect_args = self.prepare_effect(new_atom, cond_index)
    ordered_common_args = [new_atom.args[position]
                           for position in self.common_var_positions[cond_index]]
    key = tuple(ordered_common_args)
    other_cond_index = 1 - cond_index
    other_cond = self.conditions[other_cond_index]
    for atom in self.atoms_by_key[other_cond_index].get(key, []):
      for var_no, obj in zip(other_cond.args, atom.args):
        if isinstance(var_no, int):
          effect_args[var_no] = obj
      enqueue_func(self.effect.predicate, effect_args)

class ProductRule(BuildRule):
  def __init__(self, effect, conditions):
    self.effect = effect
    self.conditions = conditions
    self.atoms_by_index = [[] for c in self.conditions]
    self.empty_atom_list_no = len(self.conditions)
  def validate(self):
    assert len(self.conditions) >= 2, self
    cond_vars = [set([v for v in cond.args if isinstance(v, int) or v[0] == "?"])
                 for cond in self.conditions]
    all_cond_vars = reduce(set.union, cond_vars)
    eff_vars = set([v for v in self.effect.args
                    if isinstance(v, int) or v[0] == "?"])
    assert len(all_cond_vars) == len(eff_vars), self
    assert len(all_cond_vars) == sum([len(c) for c in cond_vars])
  def update_index(self, new_atom, cond_index):
    atom_list = self.atoms_by_index[cond_index]
    if not atom_list:
      self.empty_atom_list_no -= 1
    atom_list.append(new_atom)
  def fire(self, new_atom, cond_index, enqueue_func):
    if not self.empty_atom_list_no:
      effect_args = self.prepare_effect(new_atom, cond_index)
      self._fire(cond_index, 0, effect_args, enqueue_func)
  def _fire(self, ignore_pos, position, eff_args, enqueue_func):
    if position == len(self.conditions):
      enqueue_func(self.effect.predicate, eff_args)
    elif position == ignore_pos:
      self._fire(ignore_pos, position + 1, eff_args, enqueue_func)
    else:
      cond = self.conditions[position]
      for atom in self.atoms_by_index[position]:
        for var_no, obj in zip(cond.args, atom.args):
          if isinstance(var_no, int):
            eff_args[var_no] = obj
        self._fire(ignore_pos, position + 1, eff_args, enqueue_func)

class ProjectRule(BuildRule):
  def __init__(self, effect, conditions):
    self.effect = effect
    self.conditions = conditions
  def validate(self):
    assert len(self.conditions) == 1
  def update_index(self, new_atom, cond_index):
    pass
  def fire(self, new_atom, cond_index, enqueue_func):
    effect_args = self.prepare_effect(new_atom, cond_index)
    enqueue_func(self.effect.predicate, effect_args)

class Unifier:
  def __init__(self, rules):
    self.predicate_to_rule_generator = {}
    for rule in rules:
      for i, cond in enumerate(rule.conditions):
        self._insert_condition(rule, i)
  def unify(self, atom):
    result = []
    generator = self.predicate_to_rule_generator.get(atom.predicate)
    if generator:
      generator.generate(atom, result)
    return result
  def _insert_condition(self, rule, cond_index):
    condition = rule.conditions[cond_index]
    root = self.predicate_to_rule_generator.get(condition.predicate)
    if not root:
      root = LeafGenerator()
    constant_arguments = [(arg_index, arg)
                          for (arg_index, arg) in enumerate(condition.args)
                          if not isinstance(arg, int) and arg[0] != "?"]
    newroot = root._insert(constant_arguments, (rule, cond_index))
    self.predicate_to_rule_generator[condition.predicate] = newroot
  def dump(self):
    predicates = self.predicate_to_rule_generator.keys()
    predicates.sort()
    print "Unifier:"
    for pred in predicates:
      print "  %s:" % pred
      rule_gen = self.predicate_to_rule_generator[pred]
      rule_gen.dump(())
    
class LeafGenerator:
  index = sys.maxint
  def __init__(self):
    self.matches = []
  def generate(self, atom, result):
    result += self.matches
  def _insert(self, args, value):
    if not args:
      self.matches.append(value)
      return self
    else:
      root = LeafGenerator()
      root.matches.append(value)
      for arg_index, arg in args[::-1]:
        new_root = MatchGenerator(arg_index, LeafGenerator())
        new_root.match_generator[arg] = root
        root = new_root
      root.matches = self.matches # can be swapped in C++
      return root
  def dump(self, conditions):
    spaces = "  " + "  " * len(conditions)
    if conditions:
      print "%s%s" % (spaces, ", ".join(conditions))
    for match in self.matches:
      print "%s  %s" % (spaces, match)

class MatchGenerator:
  def __init__(self, index, next):
    self.index = index
    self.matches = []
    self.match_generator = {}
    self.next = next
  def generate(self, atom, result):
    result += self.matches
    generator = self.match_generator.get(atom.args[self.index])
    if generator:
      generator.generate(atom, result)
    self.next.generate(atom, result)
  def _insert(self, args, value):
    if not args:
      self.matches.append(value)
      return self
    else:
      arg_index, arg = args[0]
      if self.index < arg_index:
        self.next = self.next._insert(args[1:], value)
      elif self.index > arg_index:
        new_parent = MatchGenerator(arg_index, self)
        new_branch = LeafGenerator()._insert(args[1:], value)
        new_parent.match_generator[arg] = new_branch
        return new_parent
      else:
        branch_generator = self.match_generator.get(arg)
        if not branch_generator:
          branch_generator = LeafGenerator()
        self.match_generator[arg] = branch_generator._insert(args[1:], value)
        return self
  def dump(self, conditions):
    spaces = "  " + "  " * len(conditions)
    if conditions:
      print "%s%s" % (spaces, ", ".join(conditions))
    for match in self.matches:
      print "%s  %s" % (spaces, match)
    self.next.dump(conditions)
    keys = self.match_generator.keys()
    keys.sort()
    for key in keys:
      condition = "%s: %s" % (self.index, key)
      self.match_generator[key].dump(conditions + (condition,))

class Queue:
  def __init__(self, atoms):
    self.queue = atoms
    self.queue_pos = 0
    self.enqueued = set([(atom.predicate,) + tuple(atom.args)
                         for atom in self.queue])
  def __nonzero__(self):
    return self.queue_pos < len(self.queue)
  def push(self, predicate, args):
    eff_tuple = (predicate,) + tuple(args)
    if eff_tuple not in self.enqueued:
      self.enqueued.add(eff_tuple)
      self.queue.append(pddl.Atom(predicate, list(args)))
  def pop(self):
    result = self.queue[self.queue_pos]
    self.queue_pos += 1
    return result
  def popped_elements(self):
    return queue.queue[:self.queue_pos]

def compute_model(prog):
  rules = convert_rules(prog)
  unifier = Unifier(rules)
  # unifier.dump()
  queue = Queue([fact.atom for fact in prog.facts])
  print "Starting instantiation [%d rules]..." % len(rules)
  while queue:
    next_atom = queue.pop()
    matches = unifier.unify(next_atom)
    for rule, cond_index in matches:
      rule.update_index(next_atom, cond_index)
      rule.fire(next_atom, cond_index, queue.push)
  return queue.queue

if __name__ == "__main__":
  import pddl_to_prolog
  print "Parsing..."
  task = pddl.open()
  print "Writing rules..."
  prog = pddl_to_prolog.translate(task)
  print "Computing model..."
  for atom in compute_model(prog):
    print atom
