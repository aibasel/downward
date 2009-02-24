#! /usr/bin/env python2.5
# -*- coding: utf-8 -*-

import instantiate
import pddl


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


def literal_to_name(literal):
    assert not literal.negated
    return "%s(%s)" % (literal.predicate, ",".join(literal.args))


def build_relaxed_action(action):
    def fail():
        raise SystemExit("not a STRIPS action: %s" % action.name)
    preconditions = []
    effects = []
    for literal in action.precondition:
        if literal.negated:
            fail()
        preconditions.append(literal_to_name(literal))
    for conditions, effect_literal in action.add_effects:
        if conditions:
            fail()
        effects.append(literal_to_name(effect_literal))
    for conditions, effect_literal in action.del_effects:
        if conditions:
            fail()
    return RelaxedAction(action.name, sorted(preconditions), sorted(effects), 1)


def collect_init_facts(fluent_atoms, init):
    facts = []
    for atom in init:
        if atom in fluent_atoms:
            facts.append(literal_to_name(atom))
    return sorted(facts)


def collect_goal_facts(fluent_atoms, goal):
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
            if condition in fluent_atoms:
                facts.append(literal_to_name(condition))
        elif not isinstance(condition, pddl.Truth):
            fail()
    recurse(goal)
    return sorted(facts)


def build_relaxed_task(task):
    relaxed_reachable, fluent_atoms, actions, axioms = instantiate.explore(task)
    if not relaxed_reachable:
        raise SystemExit("goal is not relaxed reachable")
    if axioms:
        raise SystemExit("axioms not supported")
    relaxed_actions = [build_relaxed_action(action) for action in actions]
    init = collect_init_facts(fluent_atoms, task.init)
    goal = collect_goal_facts(fluent_atoms, task.goal)
    return RelaxedTask(fluent_atoms, init, goal, relaxed_actions)


if __name__ == "__main__":
    task = pddl.open()
    relaxed_task = build_relaxed_task(task)
    relaxed_task.dump()
