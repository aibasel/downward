# -*- coding: latin-1 -*-

import copy

import conditions
import effects
import pddl_types

class Action(object):
    def __init__(self, name, parameters, num_external_parameters,
                 precondition, effects, cost):
        assert 0 <= num_external_parameters <= len(parameters)
        self.name = name
        self.parameters = parameters
        # num_external_parameters denotes how many of the parameters
        # are "external", i.e., should be part of the grounded action
        # name. Usually all parameters are external, but "invisible"
        # parameters can be created when compiling away existential
        # quantifiers in conditions.
        self.num_external_parameters = num_external_parameters
        self.precondition = precondition
        self.effects = effects
        self.cost = cost
        self.uniquify_variables() # TODO: uniquify variables in cost?
    def __repr__(self):
        return "<Action %r at %#x>" % (self.name, id(self))
    def parse(alist):
        iterator = iter(alist)
        assert iterator.next() == ":action"
        name = iterator.next()
        parameters_tag_opt = iterator.next()
        if parameters_tag_opt == ":parameters":
            parameters = pddl_types.parse_typed_list(iterator.next(),
                                                     only_variables=True)
            precondition_tag_opt = iterator.next()
        else:
            parameters = []
            precondition_tag_opt = parameters_tag_opt
        if precondition_tag_opt == ":precondition":
            precondition = conditions.parse_condition(iterator.next())
            precondition = precondition.simplified()
            effect_tag = iterator.next()
        else:
            precondition = conditions.Conjunction([])
            effect_tag = precondition_tag_opt
        assert effect_tag == ":effect"
        effect_list = iterator.next()
        eff = []
        try:
            cost = effects.parse_effects(effect_list, eff)
        except ValueError, e:
            raise SystemExit("Error in Action %s\nReason: %s." % (name, e))
        for rest in iterator:
            assert False, rest
        return Action(name, parameters, len(parameters),
                      precondition, eff, cost)
    parse = staticmethod(parse)
    def dump(self):
        print "%s(%s)" % (self.name, ", ".join(map(str, self.parameters)))
        print "Precondition:"
        self.precondition.dump()
        print "Effects:"
        for eff in self.effects:
            eff.dump()
        print "Cost:"
        if(self.cost):
            self.cost.dump()
        else:
            print "  None"
    def uniquify_variables(self):
        self.type_map = dict([(par.name, par.type) for par in self.parameters])
        self.precondition = self.precondition.uniquify_variables(self.type_map)
        for effect in self.effects:
            effect.uniquify_variables(self.type_map)
    def unary_actions(self):
        # TODO: An neue Effect-Repräsentation anpassen.
        result = []
        for i, effect in enumerate(self.effects):
            unary_action = copy.copy(self)
            unary_action.name += "@%d" % i
            if isinstance(effect, effects.UniversalEffect):
                # Careful: Create a new parameter list, the old one might be shared.
                unary_action.parameters = unary_action.parameters + effect.parameters
                effect = effect.effect
            if isinstance(effect, effects.ConditionalEffect):
                unary_action.precondition = conditions.Conjunction([unary_action.precondition,
                                                                    effect.condition]).simplified()
                effect = effect.effect
            unary_action.effects = [effect]
            result.append(unary_action)
        return result
    def relaxed(self):
        new_effects = []
        for eff in self.effects:
            relaxed_eff = eff.relaxed()
            if relaxed_eff:
                new_effects.append(relaxed_eff)
        return Action(self.name, self.parameters, self.num_external_parameters,
                      self.precondition.relaxed().simplified(),
                      new_effects)
    def untyped(self):
        # We do not actually remove the types from the parameter lists,
        # just additionally incorporate them into the conditions.
        # Maybe not very nice.
        result = copy.copy(self)
        parameter_atoms = [par.to_untyped_strips() for par in self.parameters]
        new_precondition = self.precondition.untyped()
        result.precondition = conditions.Conjunction(parameter_atoms + [new_precondition])
        result.effects = [eff.untyped() for eff in self.effects]
        return result
    def untyped_strips_preconditions(self):
        # Used in instantiator for converting unary actions into prolog rules.
        return [par.to_untyped_strips() for par in self.parameters] + \
               self.precondition.to_untyped_strips()

    def instantiate(self, var_mapping, init_facts, fluent_facts, objects_by_type):
        """Return a PropositionalAction which corresponds to the instantiation of
        this action with the arguments in var_mapping. Only fluent parts of the
        conditions (those in fluent_facts) are included. init_facts are evaluated
        whilte instantiating.
        Precondition and effect conditions must be normalized for this to work.
        Returns None if var_mapping does not correspond to a valid instantiation
        (because it has impossible preconditions or an empty effect list.)"""
        arg_list = [var_mapping[par.name]
                    for par in self.parameters[:self.num_external_parameters]]
        name = "(%s %s)" % (self.name, " ".join(arg_list))

        precondition = []
        try:
            self.precondition.instantiate(var_mapping, init_facts,
                                          fluent_facts, precondition)
        except conditions.Impossible:
            return None
        effects = []
        for eff in self.effects:
            eff.instantiate(var_mapping, init_facts, fluent_facts,
                            objects_by_type, effects)
        if effects:
            if self.cost is None:
                cost = 0
            else:
                cost = int(self.cost.instantiate(var_mapping, init_facts).expression.value)
            return PropositionalAction(name, precondition, effects, cost)
        else:
            return None

class PropositionalAction:
    def __init__(self, name, precondition, effects, cost):
        self.name = name
        self.precondition = precondition
        self.add_effects = []
        self.del_effects = []
        for condition, effect in effects:
            if not effect.negated:
                self.add_effects.append((condition, effect))
        # Warning: This is O(N^2), could be turned into O(N).
        # But that might actually harm performance, since there are
        # usually few effects.
        # TODO: Measure this in critical domains, then use sets if acceptable.
        for condition, effect in effects:
            if effect.negated and (condition, effect.negate()) not in self.add_effects:
                self.del_effects.append((condition, effect.negate()))
        self.cost = cost
    def dump(self):
        print self.name
        for fact in self.precondition:
            print "PRE: %s" % fact
        for cond, fact in self.add_effects:
            print "ADD: %s -> %s" % (", ".join(map(str, cond)), fact)
        for cond, fact in self.del_effects:
            print "DEL: %s -> %s" % (", ".join(map(str, cond)), fact)
        print "cost:", self.cost
