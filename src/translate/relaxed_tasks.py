#! /usr/bin/env python
# -*- coding: utf-8 -*-

import instantiate
import pddl


class RelaxedAtom(object):
    def __init__(self, name):
        self.name = name

    def __cmp__(self, other):
        return cmp(self.name, other.name)

    def __repr__(self):
        return "<RelaxedAtom %r>" % self.name

    def __str__(self):
        return str(self.name)


class RelaxedAction(object):
    def __init__(self, name, preconditions, effects, cost):
        self.name = name
        self.preconditions = preconditions
        self.effects = effects
        self.cost = cost

    def dump(self):
        print "ACTION %s (cost %d):" % (self.name, self.cost)
        print "PRE:"
        for fact in self.preconditions:
            print "    %s" % fact
        print "EFF:"
        for fact in self.effects:
            print "    %s" % fact


class RelaxedTask(object):
    def __init__(self, atoms, init, goals, actions):
        self.atoms = atoms
        self.init = init
        self.goals = goals
        self.actions = actions

    def convert_to_canonical_form(self):
        # Transforms the relaxed task into an equivalent one such that:
        # * Each relaxed plan begins with action "@@init-action".
        # * Each non-redundant relaxed plan ends with action "@@goal-action".
        # * There is exactly one initial fact, "@@init".
        # * There is exactly one goal fact, "@@goal".
        # * Each action has at least one precondition.

        init_fact = RelaxedAtom("@@init")
        old_init = list(self.init)
        self.atoms.append(init_fact)
        self.init = [init_fact]
        init_action = RelaxedAction(
            name="@@init-action",
            preconditions=[init_fact],
            effects=old_init,
            cost=0)
        self.actions.append(init_action)

        goal_fact = RelaxedAtom("@@goal")
        old_goals = list(self.goals)
        self.atoms.append(goal_fact)
        self.goals = [goal_fact]
        self.actions.append(RelaxedAction(
            name="@@goal-action",
            preconditions=old_goals,
            effects=[goal_fact],
            cost=0))

        artificial_precondition = None
        for action in self.actions:
            if not action.preconditions:
                if not artificial_precondition:
                    artificial_precondition = RelaxedAtom("@@precond")
                    self.atoms.append(artificial_precondition)
                    init_action.effects.append(artificial_precondition)
                action.preconditions.append(artificial_precondition)

    def dump(self):
        print "ATOMS:"
        for fact in self.atoms:
            print "    %s" % fact
        print
        print "INIT:"
        for fact in self.init:
            print "    %s" % fact
        print
        print "GOAL:"
        for fact in self.goals:
            print "    %s" % fact
        for action in self.actions:
            print
            action.dump()


def build_relaxed_action(action, symtable):
    def fail():
        raise SystemExit("not a STRIPS action: %s" % action.name)
    preconditions = []
    effects = []
    for literal in action.precondition:
        if literal.negated:
            fail()
        preconditions.append(symtable[literal])
    for conditions, effect_literal in action.add_effects:
        if conditions:
            fail()
        effects.append(symtable[effect_literal])
    for conditions, effect_literal in action.del_effects:
        if conditions:
            fail()
    return RelaxedAction(action.name, sorted(preconditions), sorted(effects), 1)


def collect_init_facts(init, symtable):
    facts = []
    for atom in init:
        fact = symtable.get(atom)
        if fact:
            facts.append(fact)
    return sorted(facts)


def collect_goal_facts(goal, symtable):
    def fail():
        raise SystemExit("not a STRIPS goal: %s" % goal)
    facts = []
    def recurse(condition):
        if isinstance(condition, pddl.Conjunction):
            for part in condition.parts:
                recurse(part)
        elif isinstance(condition, pddl.Literal):
            if condition.negated:
                fail()
            fact = symtable.get(condition)
            if fact:
                facts.append(fact)
        elif not isinstance(condition, pddl.Truth):
            fail()
    recurse(goal)
    return sorted(facts)


def build_relaxed_task(task):
    relaxed_reachable, fluent_atoms, actions, axioms, _ = instantiate.explore(task)
    if not relaxed_reachable:
        raise SystemExit("goal is not relaxed reachable")
    if axioms:
        raise SystemExit("axioms not supported")
    symtable = dict((atom, RelaxedAtom(str(atom))) for atom in fluent_atoms)
    relaxed_atoms = sorted(symtable.values())
    relaxed_actions = [build_relaxed_action(action, symtable)
                       for action in actions]
    relaxed_init = collect_init_facts(task.init, symtable)
    relaxed_goal = collect_goal_facts(task.goal, symtable)
    return RelaxedTask(relaxed_atoms, relaxed_init, relaxed_goal,
                       relaxed_actions)


if __name__ == "__main__":
    task = pddl.open()
    relaxed_task = build_relaxed_task(task)
    relaxed_task.convert_to_canonical_form()
    relaxed_task.dump()
