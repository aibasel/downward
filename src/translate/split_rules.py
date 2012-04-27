# split_rules: Split rules whose conditions fall into different "connected
# components" (where to conditions are related if they share a variabe) into
# several rules, one for each connected component and one high-level rule.

import copy

from pddl_to_prolog import Rule, get_variables
import graph
import greedy_join
import pddl

def get_connected_conditions(conditions):
    agraph = graph.Graph(conditions)
    var_to_conditions = dict([(var, [])
                              for var in get_variables(conditions)])
    for cond in conditions:
        for var in cond.args:
            if var[0] == "?":
                var_to_conditions[var].append(cond)

    # Connect conditions with a common variable
    for var, conds in var_to_conditions.items():
        for cond in conds[1:]:
            agraph.connect(conds[0], cond)
    # TODO: Sort return value before returning it? var_to_conditions and
    #       graph.Graph.connected_components() are nondeterministic
    return agraph.connected_components()

def project_rule(rule, conditions, name_generator):
    predicate = next(name_generator)
    effect_variables = set(rule.effect.args) & get_variables(conditions)
    effect = pddl.Atom(predicate, sorted(effect_variables))
    projected_rule = Rule(conditions, effect)
    return projected_rule

def split_rule(rule, name_generator):
    important_conditions, trivial_conditions = [], []
    for cond in rule.conditions:
        for arg in cond.args:
            if arg[0] == "?":
                important_conditions.append(cond)
                break
        else:
            trivial_conditions.append(cond)

    # important_conditions = [cond for cond in rule.conditions if cond.args]
    # trivial_conditions = [cond for cond in rule.conditions if not cond.args]

    components = get_connected_conditions(important_conditions)
    if len(components) == 1 and not trivial_conditions:
        return split_into_binary_rules(rule, name_generator)

    projected_rules = [project_rule(rule, conditions, name_generator)
                       for conditions in components]
    result = []
    for proj_rule in projected_rules:
        result += split_into_binary_rules(proj_rule, name_generator)

    conditions = [proj_rule.effect for proj_rule in projected_rules] + \
                 trivial_conditions
    combining_rule = Rule(conditions, rule.effect)
    if len(conditions) >= 2:
        combining_rule.type = "product"
    else:
        combining_rule.type = "project"
    result.append(combining_rule)
    return result

def split_into_binary_rules(rule, name_generator):
    if len(rule.conditions) <= 1:
        rule.type = "project"
        return [rule]
    return greedy_join.greedy_join(rule, name_generator)
