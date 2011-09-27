#! /usr/bin/env python
# -*- coding: utf-8 -*-

import axiom_rules
import fact_groups
import instantiate
import pddl
import sas_tasks
import simplify

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
USE_PARTIAL_ENCODING = True

def strips_to_sas_dictionary(groups):
    dictionary = {}
    for var_no, group in enumerate(groups):
        for val_no, atom in enumerate(group):
            dictionary.setdefault(atom, []).append((var_no, val_no))
    if USE_PARTIAL_ENCODING:
        assert all(len(sas_pairs) == 1
                   for sas_pairs in dictionary.itervalues())
    return [len(group) + 1 for group in groups], dictionary

def translate_strips_conditions(conditions, dictionary, ranges):
    if not conditions:
        return {} # Quick exit for common case.

    condition = {}
    for fact in conditions:
        atom = pddl.Atom(fact.predicate, fact.args) # force positive
        for var, val in dictionary[atom]:
            if fact.negated:
                assert False, "neg. precondition: task not in positive normal form"
            if condition.get(var) not in (None, val):
                # Conflicting conditions on this variable: Operator invalid.
                return None
            condition[var] = val
    return condition

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
        for var, val in dictionary[fact]:
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
        for var, val in dictionary[fact]:
            none_of_those = ranges[var] - 1

            other_val, eff_conditions = effect.get(var, (none_of_those, []))

            if other_val != none_of_those:
                # Look for matching add effect; ignore this del effect if found.
                assert eff_condition in eff_conditions or [] in eff_conditions, \
                              "Add effect with uncertain del effect partner?"
                if other_val == val:
                    if ALLOW_CONFLICTING_EFFECTS:
                        # Conflicting ADD and DEL effect. This is *only* allowed if
                        # this is also a precondition, in which case there is *no*
                        # effect (the ADD takes precedence). We delete the add effect here.
                        if condition.get(var) != val:
                            # HACK HACK HACK!
                            # There used to be an assertion here that actually
                            # forbid this, but this was wrong in Pipesworld-notankage
                            # (e.g. task 01). The thing is, it *is* possible for
                            # an operator with proven (with the given invariants)
                            # inconsistent preconditions to actually end up here if
                            # the inconsistency of the preconditions is not obvious at
                            # the SAS+ encoding level.
                            #
                            # Example: Pipes-Notank-01, operator
                            # (pop-unitarypipe s12 b4 a1 a2 b4 lco lco).
                            # This has precondition last(b4, s12) and on(b4, a2) which
                            # is inconsistent because b4 can only be in one place.
                            # However, the chosen encoding encodes *what is last in s12*
                            # separately, and so the precondition translates to
                            # "last(s12) = b4 and on(b4) = a2", which does not look
                            # inconsistent at first glance.
                            #
                            # Something reasonable to do here would be to make a
                            # decent check that the precondition is indeed inconsistent
                            # (using *all* mutexes), but that seems tough with this
                            # convoluted code, so we just warn and reject the operator.
                            print "Warning: %s rejected. Cross your fingers." % (
                                operator.name)
                            return None
                            assert False

                        assert eff_conditions == [[]]
                        del effect[var]
                    else:
                        assert not eff_condition and not eff_conditions[0], "Uncertain conflict"
                        return None  # Definite conflict otherwise.
#            else:
#                if condition.get(var) != val and eff_condition_dict.get(var) != val:
#                    # Need a guard for this delete effect.
#                    assert var not in condition and var not in eff_condition, "Oops?"
#                    eff_condition.append((var, val))
#                eff_conditions.append(eff_condition)

    if possible_add_conflict:
        print operator.name

    assert not possible_add_conflict, "Conflicting add effects?"

    pre_post = []
    for var, (post, eff_condition_lists) in effect.iteritems():
        pre = condition.get(var, -1)
        if pre != -1:
            del condition[var]
        for eff_condition in eff_condition_lists:
            pre_post.append((var, pre, post, eff_condition))
    prevail = condition.items()

    return sas_tasks.SASOperator(operator.name, prevail, pre_post, operator.cost)

def translate_strips_axiom(axiom, dictionary, ranges):
    condition = translate_strips_conditions(axiom.condition, dictionary, ranges)
    if condition is None:
        return None
    if axiom.effect.negated:
        [(var, _)] = dictionary[axiom.effect.positive()]
        effect = (var, ranges[var] - 1)
    else:
        [effect] = dictionary[axiom.effect]
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

def translate_task(strips_to_sas, ranges, translation_key, mutex_key,
                   init, goals, actions, axioms, metric):
    axioms, axiom_init, axiom_layer_dict = axiom_rules.handle_axioms(
      actions, axioms, goals)
    init = init + axiom_init
    #axioms.sort(key=lambda axiom: axiom.name)
    #for axiom in axioms:
    #  axiom.dump()

    init_values = [rang - 1 for rang in ranges]
    # Closed World Assumption: Initialize to "range - 1" == Nothing.
    for fact in init:
        pair = strips_to_sas.get(fact)
        pairs = strips_to_sas.get(fact, [])  # empty for static init facts
        for var, val in pairs:
            assert init_values[var] == ranges[var] - 1, "Inconsistent init facts!"
            init_values[var] = val
    init = sas_tasks.SASInit(init_values)

    goal_pairs = translate_strips_conditions(goals, strips_to_sas, ranges).items()
    goal = sas_tasks.SASGoal(goal_pairs)

    operators = translate_strips_operators(actions, strips_to_sas, ranges)
    axioms = translate_strips_axioms(axioms, strips_to_sas, ranges)

    axiom_layers = [-1] * len(ranges)
    for atom, layer in axiom_layer_dict.iteritems():
        assert layer >= 0
        [(var, val)] = strips_to_sas[atom]
        axiom_layers[var] = layer
    variables = sas_tasks.SASVariables(ranges, axiom_layers, translation_key)
    mutexes = [sas_tasks.SASMutexGroup(group) for group in mutex_key]
    return sas_tasks.SASTask(variables, mutexes, init, goal,
                             operators, axioms, metric)

def unsolvable_sas_task(msg):
    print "%s! Generating unsolvable task..." % msg
    variables = sas_tasks.SASVariables([2], [-1])
    init = sas_tasks.SASInit([0])
    goal = sas_tasks.SASGoal([(0, 1)])
    operators = []
    axioms = []
    return sas_tasks.SASTask(variables, init, goal, operators, axioms)

def pddl_to_sas(task):
    print "Instantiating..."
    relaxed_reachable, atoms, actions, axioms, _ = instantiate.explore(task)

    if not relaxed_reachable:
        return unsolvable_sas_task("No relaxed solution")

    # HACK! Goals should be treated differently (see TODO file).
    if isinstance(task.goal, pddl.Conjunction):
        goal_list = task.goal.parts
    else:
        goal_list = [task.goal]
    for item in goal_list:
        assert isinstance(item, pddl.Literal)

    # switched off invariant syntheses -> one group for each fluent fact
    groups = [[fact] for fact in atoms]
    mutex_groups = []
    translation_key = [[str(fact),str(fact.negate())] for group in groups
                                                      for fact in group]

    print "Building STRIPS to SAS dictionary..."
    ranges, strips_to_sas = strips_to_sas_dictionary(groups)

    print "Building mutex information..."
    mutex_key = build_mutex_key(strips_to_sas, mutex_groups)

    print "Translating task..."
    sas_task = translate_task(strips_to_sas, ranges, translation_key,
                              mutex_key, task.init, goal_list, actions, axioms,
                              task.use_min_cost_metric)

    try:
        simplify.filter_unreachable_propositions(sas_task)
    except simplify.Impossible:
        return unsolvable_sas_task("Simplified to trivially false goal")

    return sas_task

def build_mutex_key(strips_to_sas, groups):
    group_keys = []
    for group in groups:
        group_key = []
        for fact in group:
            if strips_to_sas.get(fact):
                for var, val in strips_to_sas[fact]:
                    group_key.append((var, val, str(fact)))
            else:
                print "not in strips_to_sas, left out:", fact
        group_keys.append(group_key)
    return group_keys

if __name__ == "__main__":
    import pddl
    print "Parsing..."
    task = pddl.open()
    if task.domain_name in ["protocol", "rover"]:
        # This is, of course, a HACK HACK HACK!
        # The real issue is that ALLOW_CONFLICTING_EFFECTS = True
        # is actually the correct semantics, but then we don't get to filter
        # out operators that are impossible to apply due to mutexes between
        # different SAS+ variables. For example,
        # ALLOW_CONFLICTING_EFFECTS = True does not filter on(a,a) in
        # blocksworld/4-0.
        ALLOW_CONFLICTING_EFFECTS = True

    # EXPERIMENTAL!
    # import psyco
    # psyco.full()

    sas_task = pddl_to_sas(task)
    print "Writing output..."
    sas_task.output(file("output.sas", "w"))
    print "Done!"
