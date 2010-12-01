# -*- coding: latin-1 -*-

import pddl
import tools
import copy
import random # only for tests

# TODO: Rewrite.
#
# Ziel:
# - Die Abhängigkeit von der STRIPS-Kodierung muss reduziert bzw. in einem
#   kleinen Teil gekapselt werden.
# - Der Zusammenhang zwischen dem Algorithmus und dem logischen Gehalt der
#   Invarianten (siehe LaTeX-Ausdruck) muss klarer werden.
#
# Ideen:
# Invarianten sollten ihre eigene Parameter-Zahl ("arity") kennen, nicht nur
# indirekt über ihre parts erfahren können. Der Test der Erfülltheit einer
# Invariante für eine bestimmte Aktion könnte so aussehen:
# a) Gehe die Add-Effekte der Aktion durch und überprüfe, ob sie einen Teil
#    der Invariante betreffen. Wenn ja, merke Dir die Parameter für die
#    Invariante sowie den zugehörigen Add-Effekt-Fakt und dessen Condition.
#    Wenn dieselben Parameter mehrfach auftreten, verwirf die Invariante.
#    (Problem mit doppelten Add-Effekten; kann man vorher weg-normalisieren,
#     selbst bei unterschiedlichen conditions).
#    Ebenso verwerfen, wenn die X-Variable eines Invariantenteils im Effekt
#    universell quantifiziert ist.
#    Beispiel:
#      Forall X:  sum_Z at(X, Z) + sum_Z in(Z, X) <= 1.
#    Add-Effekt: at(?v, ?w) => Merke dir den Parameter ?v.
#    Add-Effekt: in(?v, ?w) => Merke dir den Parameter ?w.
# b) Gehe analog die Delete-Effekte der Aktion durch und streiche die
#    Parameter aus der Liste, deren assoziierte Conditions die Condition des
#    Delete-Effekts implizieren (so dass der Delete-Effekt tatsächlich
#    gelöscht wird) und auch, zusammen mit der Vorbedingung des Operators, den
#    Delete-Fakt selbst implizieren (da sonst gar nicht unbedingt etwas
#    gelöscht wird).
# Bleiben dann noch Dinge übrig, gehe die Delete-Effekte erneut durch und
# versuche Matchings zu finden.
#
# Hmm... leider alles sehr schwammig. :-(
# Bis Ende der Woche sollte der Algorithmus auf den ADL-Domänen funktionieren.

def invert_list(alist):
    result = {}
    for pos, arg in enumerate(alist):
        result.setdefault(arg, []).append(pos)
    return result

def instantiate_factored_mapping(pairs):
    part_mappings = [[zip(preimg, perm_img) for perm_img in tools.permutations(img)]
                     for (preimg, img) in pairs]
    return tools.cartesian_product(part_mappings)

class NegativeClause(object):
    def __init__(self, parts):
        self.parts = parts
    def is_satisfiable(self):
        for part in self.parts:
            if part[0] == part[1]:
                return False
        return True
    def apply_assignment(self, assignment):
        m = assignment.get_mapping()
        if not m:
            return None
        new_parts = [(m.get(v1, v1), m.get(v2, v2)) for (v1, v2) in parts]
        return NegativeClause(new_parts)

class Assignment(object):
    def __init__(self, equalities):
        self.equalities = equalities 
        # represents a conjunction of expressions ?x = ?y or ?x = d
        # with ?x, ?y being variables and d being a domain value
    def get_mapping(self):
        eq_classes = dict()
        for (v1, v2) in self.equalities:
            c1 = eq_classes.setdefault(v1, set([v1]))
            c2 = eq_classes.setdefault(v2, set([v2]))
            if id(c1) != id(c2):
                if len(c2) > len(c1):
                    v1, c1, v2, c2 = v2, c2, v1, c1
                c1.update(c2)
                for elem in c2:
                    eq_classes[elem] = c1
        all_classes = set([frozenset(s) for s in eq_classes.values()])
        mapping = dict()
        for eq_class in all_classes:
            current = list(eq_class)
            current.sort()
            maxval = current[-1]
            if len(current) > 1 and not current[-2].startswith("?"):
                return None # inconsistent Assignment (d1 = d2)
            for entry in current:
                mapping[entry] = maxval
        return mapping

class InvariantPart:
    def __init__(self, predicate, order, omitted_pos=-1):
        self.predicate = predicate
        self.order = order
        self.omitted_pos = omitted_pos
    def __eq__(self, other):
        # This implies equality of the omitted_pos component.
        return self.predicate == other.predicate and self.order == other.order
    def __ne__(self, other):
        return self.predicate != other.predicate or self.order != other.order
    def __hash__(self):
        return hash((self.predicate, tuple(self.order)))
    def __str__(self):
        var_string = " ".join(map(str, self.order))
        omitted_string = ""
        if self.omitted_pos != -1:
            omitted_string = " [%d]" % self.omitted_pos
        return "%s %s%s" % (self.predicate, var_string, omitted_string)
    def get_parameters(self, literal):
        return [literal.args[pos] for pos in self.order]
    def instantiate(self, parameters):
        args = ["?X"] * (len(self.order) + (self.omitted_pos != -1))
        for arg, argpos in zip(parameters, self.order):
            args[argpos] = arg
        return pddl.Atom(self.predicate, args)
    def possible_mappings(self, own_literal, other_literal):
        allowed_omissions = len(other_literal.args) - len(self.order)
        if allowed_omissions not in (0, 1):
            return []
        own_parameters = self.get_parameters(own_literal)
        arg_to_ordered_pos = invert_list(own_parameters)
        other_arg_to_pos = invert_list(other_literal.args)
        factored_mapping = []

        for key, other_positions in other_arg_to_pos.iteritems():
            own_positions = arg_to_ordered_pos.get(key, [])
            len_diff = len(own_positions) - len(other_positions)
            if len_diff >= 1 or len_diff <= -2 or len_diff == -1 and not allowed_omissions:
                return []
            if len_diff:
                own_positions.append(-1)
                allowed_omissions = 0
            factored_mapping.append((other_positions, own_positions))
        return instantiate_factored_mapping(factored_mapping)
    def possible_matches(self, own_literal, other_literal):
        assert self.predicate == own_literal.predicate
        result = []
        for mapping in self.possible_mappings(own_literal, other_literal):
            new_order = [None] * len(self.order)
            omitted = -1
            for (key, value) in mapping:
                if value == -1:
                    omitted = key
                else:
                    new_order[value] = key
            result.append(InvariantPart(other_literal.predicate, new_order, omitted))
        return result
    def matches(self, other, own_literal, other_literal):
        return self.get_parameters(own_literal) == other.get_parameters(other_literal)

class Invariant:
    # An invariant is a logical expression of the type
    #   forall V1...Vk: sum_(part in parts) weight(part, V1, ..., Vk) <= 1.
    # k is called the arity of the invariant.
    # A "part" is a symbolic fact only variable symbols in {V1, ..., Vk, X};
    # the symbol X may occur at most once.

    def __init__(self, parts):
        self.parts = frozenset(parts)
        self.predicates = set([part.predicate for part in parts])
        self.predicate_to_part = dict([(part.predicate, part) for part in parts])
        assert len(self.parts) == len(self.predicates)
    def __eq__(self, other):
        return self.parts == other.parts
    def __ne__(self, other):
        return self.parts != other.parts
    def __hash__(self):
        return hash(self.parts)
    def __str__(self):
        return "{%s}" % ", ".join(map(str, self.parts))
    def get_parameters(self, atom):
        return self.predicate_to_part[atom.predicate].get_parameters(atom)
    def instantiate(self, parameters):
        return [part.instantiate(parameters) for part in self.parts]
    def check_balance(self, balance_checker, enqueue_func):
        # Check balance for this hypothesis.
        actions_to_check = set()
        for part in self.parts:
            actions_to_check |= balance_checker.get_threats(part.predicate)
        for action in actions_to_check:
            if self.check_operator_too_heavy(action):
                return False
#            if not self.check_action_balance(balance_checker, action, enqueue_func):
#                return False
        return False
    def check_operator_too_heavy(self, action):
        # duplicate universal effects and assign unique names to all 
        # quantified variables
        new_effects = []
        for eff in action.effects:
            new_effects.append(eff)
            if len(eff.parameters) > 0:
                new_effects.append(copy.copy(eff))
        act = pddl.Action(action.name, action.parameters, action.precondition,
                          new_effects, action.cost)

        add_effects = [eff for eff in act.effects 
                           if not eff.literal.negated and
                              self.predicate_to_part.get(eff.literal.predicate)]

        params = [p.name for p in act.parameters]
        for eff in act.effects:
            params += [p.name for p in eff.parameters]
        must_be_equal = set([]) 

        # assume add1 and add2 are two distinct add_effects

        for eff in add_effects:
            part = self.predicate_to_part.get(eff.literal.predicate)

        
    def check_action_balance(self, balance_checker, action, enqueue_func):
        # Check balance for this hypothesis with regard to one action.
        del_effects = [eff for eff in action.effects if eff.literal.negated]
        add_effects = [eff for eff in action.effects if not eff.literal.negated]
        matched_add_effects = []
        for eff in add_effects:
            part = self.predicate_to_part.get(eff.literal.predicate)
            if part:
                for previous_part, previous_literal in matched_add_effects:
                    if previous_part.matches(part, previous_literal, eff.literal) \
                       and previous_literal != eff.literal: # "Airport" has duplicate effects
                        return False
                if not self.find_matching_del_effect(part, eff, del_effects, enqueue_func):
                    return False
                matched_add_effects.append((part, eff.literal))
        return True
    def find_matching_del_effect(self, part, add_effect, del_effects, enqueue_func):
        # Check balance for this hypothesis with regard to one add effect.
        for del_eff in del_effects:
            del_part = self.predicate_to_part.get(del_eff.literal.predicate)
            if del_part and part.matches(del_part, add_effect.literal, del_eff.literal):
                return True
        # Otherwise, no match => Generate new candidates.
        for del_eff in del_effects:
            if del_eff.literal.predicate not in self.predicate_to_part:
                for match in part.possible_matches(add_effect.literal, del_eff.literal):
                    enqueue_func(Invariant(self.parts.union((match,))))
        return False # Balance check failed.

if __name__ == "__main__":
    test = [("?a", "?b"), ("?b", "d"), ("?c", "?e"), ("?c", "?f"), ("?a", "?f")]
    test2 = [("?a", "k"), ("?a", "?b"), ("?b", "d"), ("?c", "?e"), ("?c", "?f"), ("?a", "?f")]
    for case in (test, test2):
        for x in range(5):
            new_test = []
            for (v1,v2) in case:
                if v2.startswith("?") and random.randint(0, 1):
                    new_test.append((v2, v1))
                else:
                    new_test.append((v1, v2))
            print new_test
            a = Assignment(set(new_test))
            print a.get_mapping()
   
