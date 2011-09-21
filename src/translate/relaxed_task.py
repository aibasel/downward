#! /usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import StringIO

import instantiate


def explore_silently(task):
    # Call instantiate.explore, swallowing its output.
    real_stdout = sys.stdout
    sys.stdout = StringIO.StringIO()
    result = instantiate.explore(task)
    sys.stdout = real_stdout
    return result


def literal_to_name(literal):
    if literal.negated:
        raise SystemExit("negative conditions not supported")
    return "%s(%s)" % (literal.predicate, ",".join(literal.args))


def relaxed_task(actions):
    for action in actions:
        precond_names = [literal_to_name(literal)
                         for literal in action.precondition]
        for conditions, effect_literal in action.add_effects:
            if conditions:
                raise SystemExit("conditional effects not supported")
            effect_name = literal_to_name(effect_literal) + "'"
            yield ", ".join(precond_names) + " => " + effect_name


if __name__ == "__main__":
    import pddl
    task = pddl.open()
    relaxed_reachable, atoms, actions, axioms, _ = explore_silently(task)
    if not relaxed_reachable:
        raise SystemExit("goal is not relaxed reachable")
    if axioms:
        raise SystemExit("axioms not supported")

    # Generate complete output before printing it, to see if errors occur.
    out = list(relaxed_task(actions))
    for line in out:
        print line
