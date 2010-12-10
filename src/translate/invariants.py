# -*- coding: latin-1 -*-

import pddl
import tools
import copy
import itertools
import random # only for tests

# Ideen:
# Invarianten sollten ihre eigene Parameter-Zahl ("arity") kennen, nicht nur
# indirekt über ihre parts erfahren können. 

# Notes (from/to Gabi):
# All parts of an invariant aways use all non-counted variables
# -> the arity of all predicates covered by an invariant is either the
# number of the invariant variables or this value + 1
#
# we currently keep the assumption that each predicate occurs at most once
# in every invariant. This will be changed in it's own commit

def invert_list(alist):
    result = {}
    for pos, arg in enumerate(alist):
        result.setdefault(arg, []).append(pos)
    return result

def instantiate_factored_mapping(pairs):
    part_mappings = [[zip(preimg, perm_img) for perm_img in tools.permutations(img)]
                     for (preimg, img) in pairs]
    return tools.cartesian_product(part_mappings)
                
def find_unique_variables(action, invariant):
    # find unique names for invariant variables
    params = set([p.name for p in action.parameters])
    for eff in action.effects:
        params.update([p.name for p in eff.parameters])
    inv_vars = []
    need_more_variables = len(invariant.parts.__iter__().next().order)
    # TODO: aaahrg. There must be a better way of getting the
    # arity of the invariant
    if need_more_variables:
        for counter in itertools.count(1):
            new_name = "?v%i" % counter
            if new_name not in params:
                inv_vars.append(new_name)
                need_more_variables -= 1
                params.add(new_name)
                if not need_more_variables:
                    break
    return inv_vars

def get_literals(condition):
    if isinstance(condition, pddl.Literal):
        yield condition
    elif isinstance(condition, pddl.Conjunction):
        for literal in condition.parts:
                yield literal

class NegativeClause(object):
    # disjunction of inequalities
    def __init__(self, parts):
        self.parts = parts
    def is_satisfiable(self):
        for part in self.parts:
            if part[0] != part[1]:
                return True
        return False
    def apply_mapping(self, m):
        new_parts = [(m.get(v1, v1), m.get(v2, v2)) for (v1, v2) in self.parts]
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
    def get_assignment(self, parameters, literal):
        equalities = [(arg, literal.args[argpos]) 
                      for arg, argpos in zip(parameters, self.order)] 
        return Assignment(equalities)
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
        old_value = True
        for action in actions_to_check:
            if not self.check_action_balance(balance_checker, action, enqueue_func):
                old_value = False
                print "old:", action.name
                break

        new_value = True
        for action in actions_to_check:
            if self.operator_too_heavy(action):
                print "too heavy"
                new_value = False
                print "new:", action.name
                break
            if self.operator_unbalanced(action, enqueue_func):
                print "unbalanced"
                new_value = False
                print "new:", action.name
                break

        assert old_value == new_value, "%s %s, %s"% (old_value, new_value, self)
        return old_value
    def operator_too_heavy(self, action):
        # XXX TODO: some things can be precomputed once

        # duplicate universal effects and assign unique names to all 
        # quantified variables
        new_effects = []
        for eff in action.effects:
            new_effects.append(eff)
            if len(eff.parameters) > 0: # universal effect
                new_effects.append(copy.copy(eff))
        act = pddl.Action(action.name, action.parameters, action.precondition,
                          new_effects, action.cost)

        add_effects = [eff for eff in act.effects 
                           if not eff.literal.negated and
                              self.predicate_to_part.get(eff.literal.predicate)]

        inv_vars = find_unique_variables(act, self)
        
        for index1, eff1 in enumerate(add_effects):
            for index2 in range(index1 + 1, len(add_effects)):
                eff2 = add_effects[index2]
                negative_clauses = []
                combinatorial_assignments = []
                
                # eff1.atom != eff2.atom
                l1 = eff1.literal
                l2 = eff2.literal
                if l1.predicate == l2.predicate:
                    parts = [(l1.parts[i], l2.parts[i]) 
                             for i in range(len(l1.parts))]
                    if len(parts) != 0:
                        negative_clauses.append(NegativeClause(parts))
                
                # covers(V, Phi, eff1.atom)
                part = self.predicate_to_part[eff1.literal.predicate] 
                assignments1 = []
                assignments1.append(part.get_assignment(inv_vars, eff1.literal))
                combinatorial_assignments.append(assignments1)
                
                # covers(V, Phi, eff2.atom)
                part = self.predicate_to_part[eff2.literal.predicate] 
                assignments2 = []
                assignments2.append(part.get_assignment(inv_vars, eff2.literal))
                combinatorial_assignments.append(assignments2)

                # precondition plus effect conditions plus both literals
                # false
                pos, neg = dict(), dict()
                neg.setdefault(eff1.literal.predicate, set()).add(eff1.literal)
                neg.setdefault(eff2.literal.predicate, set()).add(eff2.literal)
                for literal in itertools.chain(get_literals(act.precondition),
                                               get_literals(eff1.condition),
                                               get_literals(eff2.condition)):
                    if literal.negated:
                        neg.setdefault(literal.predicate, set()).add(literal)
                    else:
                        pos.setdefault(literal.predicate, set()).add(literal)

                    # use (in)equalities in conditions
                    if literal.predicate == "=": 
                        if literal.negated:
                            n = NegativeClause([literal.args])
                            negative_clauses.append(n)
                        else:
                            a = Assignment([literal.args])
                            combinatorial_assignments.append(a)

                for pred, posatoms in pos.iteritems():
                    if pred in neg:
                        negatoms = neg[pred]
                        for posatom in posatoms:
                            for negatom in negatoms:
                                parts = zip(negatom.args, posatom.args)
                                if len(parts) != 0:
                                    negative_clauses.append(NegativeClause(parts))

                # check for all covering assignments whether they make the
                # conjunction of all negative_clauses unsatisfiably
                for assignments in itertools.product(*combinatorial_assignments):
                    new_equalities = reduce(lambda x,y: x + y, 
                                            [a.equalities for a in assignments])
                    mapping = Assignment(new_equalities).get_mapping()
                    if mapping != None: # otherwise the assignments are inconsistent
                        satisfiable = True
                        for neg_clause in negative_clauses:
                            clause = neg_clause.apply_mapping(mapping)
                            if not clause.is_satisfiable():
                                satisfiable = False
                                break
                        if satisfiable:
                            return True
        return False
    def operator_unbalanced(self, action, enqueue_func):
        inv_vars = find_unique_variables(action, self)
        rel_effects = [eff for eff in action.effects 
                           if self.predicate_to_part.get(eff.literal.predicate)]
        add_effects = [eff for eff in rel_effects
                           if not eff.literal.negated]
        del_effects = [eff for eff in rel_effects
                           if eff.literal.negated]
        for eff in add_effects:
            if self.add_effect_unbalanced(action, eff, del_effects, 
                                          inv_vars, enqueue_func):
                return True
        return False
    def add_effect_unbalanced(self, action, add_effect, del_effects, 
                              inv_vars, enqueue_func):

        # add_effect must be covered
        part = self.predicate_to_part[add_effect.literal.predicate] 
        assignment = part.get_assignment(inv_vars, add_effect.literal)

        # renaming of operator parameters must be minimal
        minimality_clauses = []
        params = [p.name for p in action.parameters]
        mapping = assignment.get_mapping()
        for (n1, n2) in itertools.combinations(params, 2):
            if mapping.get(n1, n1) != mapping.get(n2, n2):
                minimality_clauses.append(NegativeClause([(n1, n2)]))
       
        lhs_by_pred = dict()
        for lit in itertools.chain(get_literals(action.precondition),
                                   get_literals(add_effect.condition),
                                   get_literals(add_effect.literal.negate())):
            lhs_by_pred.setdefault(lit.predicate, []).append(lit)

        def check_del_effect(del_effect):
            """returns true if the del_effects balances the add_effect"""
            negative_clauses = []
            # add_eff.atom != del_eff.atom
            # TODO almost the same code in too heavy
            l1 = add_effect.literal
            l2 = del_effect.literal
            if l1.predicate == l2.predicate:
                parts = [(l1.parts[i], l2.parts[i]) 
                         for i in range(len(l1.parts))]
                if len(parts) != 0:
                    negative_clauses.append(NegativeClause(parts))
            
            combinatorial_assignments = [[assignment]]
            # del_effect.atom must be covered
            # XXX TODO: add one possibility for each occurence of the del pred
            # in the invariant
            part = self.predicate_to_part[del_effect.literal.predicate] 
            del_assig = [part.get_assignment(inv_vars, del_effect.literal)]
            combinatorial_assignments.append(del_assig)
            
            # del_effect.cond and del_effect.atom must be implied by lhs
            for literal in itertools.chain(get_literals(del_effect.condition),
                                           [del_effect.literal.negate()]):
                if not literal.predicate in lhs_by_pred:
                    return False
                poss_assignments = []
                for match in lhs_by_pred[literal.predicate]:
                    if match.negated != literal.negated:
                        continue
                    else:
                        a = Assignment(zip(literal.parts, match.parts))
                        poss_assignments.append(a)
                if len(poss_assignments) == 0:
                    return False
                combinatorial_assignments.append(poss_assignments)

            # check all promising renamings of the quantified effect variables
            for assignments in itertools.product(*combinatorial_assignments):
                new_equalities = reduce(lambda x,y: x + y, 
                                        [a.equalities for a in assignments])
                mapping = Assignment(new_equalities).get_mapping()
                if mapping == None:
                    continue
                found_renaming = True
                for neg_clause in itertools.chain(minimality_clauses, 
                                                  negative_clauses):
                    if not neg_clause.apply_mapping(mapping).is_satisfiable():
                        found_renaming = False
                        break
                if found_renaming:
                    return True
            return False

        for del_effect in del_effects:
            if check_del_effect(del_effect):
                return False
        # Otherwise, no match => Generate new candidates.
        part = self.predicate_to_part[add_effect.literal.predicate]
        for del_eff in [eff for eff in action.effects if eff.literal.negated]:
            if del_eff.literal.predicate not in self.predicate_to_part:
                for match in part.possible_matches(add_effect.literal, del_eff.literal):
                    enqueue_func(Invariant(self.parts.union((match,))))
        return True # balance check fails
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
        return False # Balance check failed.

