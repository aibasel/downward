#! /usr/bin/env python3


from collections import defaultdict
from typing import Any, Dict, List, Optional, Set, Tuple

import build_model
import pddl_to_prolog
import pddl
import timers

def get_fluent_facts(task, model):
    fluent_predicates = set()
    for action in task.actions:
        for effect in action.effects:
            fluent_predicates.add(effect.literal.predicate)
    for axiom in task.axioms:
        fluent_predicates.add(axiom.name)
    return {fact for fact in model
            if fact.predicate in fluent_predicates}

def get_objects_by_type(typed_objects, types):
    result = defaultdict(list)
    supertypes = {}
    for type in types:
        supertypes[type.name] = type.supertype_names
    for obj in typed_objects:
        result[obj.type_name].append(obj.name)
        for type in supertypes[obj.type_name]:
            result[type].append(obj.name)
    return result

def instantiate_goal(goal, init_facts, fluent_facts):
    # With the way this module is designed, we need to "instantiate"
    # the goal to make sure we properly deal with static conditions,
    # in particular flagging unreachable negative static goals as
    # impossible. See issue1055.
    #
    # This returns None for goals that are impossible due to static
    # facts.

    # HACK! The implementation of this probably belongs into
    # pddl.condition or a similar file, not here. The `instantiate`
    # method of conditions with its slightly weird interface and the
    # existence of the `Impossible` exceptions should perhaps be
    # implementation details of `pddl`.
    result = []
    try:
        goal.instantiate({}, init_facts, fluent_facts, result)
    except pddl.conditions.Impossible:
        return None
    return result

# The input task must have been normalized
# The model has been computed by build_model.compute_model
def instantiate(task: pddl.Task, model: Any) -> Tuple[
             bool, # relaxed_reachable
             Set[pddl.Literal], # fluent_facts (ground)
             List[pddl.PropositionalAction], # instantiated_actions
             Optional[List[pddl.Literal]], # instantiated_goal
             List[pddl.PropositionalAxiom], # instantiated_axioms
             Dict[pddl.Action, List[str]] # reachable_action_parameters
            ]:
    relaxed_reachable = False
    fluent_facts = get_fluent_facts(task, model)
    init_facts = set()
    init_assignments = {}
    for element in task.init:
        if isinstance(element, pddl.Assign):
            init_assignments[element.fluent] = element.expression
        else:
            init_facts.add(element)

    type_to_objects = get_objects_by_type(task.objects, task.types)

    instantiated_actions = []
    instantiated_axioms = []
    reachable_action_parameters = defaultdict(list)
    for atom in model:
        if isinstance(atom.predicate, pddl.Action):
            action = atom.predicate
            parameters = action.parameters
            inst_parameters = atom.args[:len(parameters)]
            # Note: It's important that we use the action object
            # itself as the key in reachable_action_parameters (rather
            # than action.name) since we can have multiple different
            # actions with the same name after normalization, and we
            # want to distinguish their instantiations.
            reachable_action_parameters[action].append(inst_parameters)
            variable_mapping = {par.name: arg
                                for par, arg in zip(parameters, atom.args)}
            inst_action = action.instantiate(
                variable_mapping, init_facts, init_assignments,
                fluent_facts, type_to_objects,
                task.use_min_cost_metric)
            if inst_action:
                instantiated_actions.append(inst_action)
        elif isinstance(atom.predicate, pddl.Axiom):
            axiom = atom.predicate
            variable_mapping = {par.name: arg
                                for par, arg in zip(axiom.parameters, atom.args)}
            inst_axiom = axiom.instantiate(variable_mapping, init_facts, fluent_facts)
            if inst_axiom:
                instantiated_axioms.append(inst_axiom)
        elif atom.predicate == "@goal-reachable":
            relaxed_reachable = True

    instantiated_goal = instantiate_goal(task.goal, init_facts, fluent_facts)

    return (relaxed_reachable, fluent_facts,
            instantiated_actions, instantiated_goal,
            sorted(instantiated_axioms), reachable_action_parameters)


def explore(task):
    prog = pddl_to_prolog.translate(task)
    model = build_model.compute_model(prog)
    with timers.timing("Completing instantiation"):
        return instantiate(task, model)


if __name__ == "__main__":
    import pddl_parser
    task = pddl_parser.open()
    relaxed_reachable, atoms, actions, goals, axioms, _ = explore(task)
    print("goal relaxed reachable: %s" % relaxed_reachable)
    print("%d atoms:" % len(atoms))
    for atom in atoms:
        print(" ", atom)
    print()
    print("%d actions:" % len(actions))
    for action in actions:
        action.dump()
        print()
    print("%d axioms:" % len(axioms))
    for axiom in axioms:
        axiom.dump()
        print()
    print()
    if goals is None:
        print("impossible goal")
    else:
        print("%d goals:" % len(goals))
        for literal in goals:
            literal.dump()
