# -*- coding: latin-1 -*-

from collections import defaultdict
import itertools

import pddl
import tools

# Ideen:
# Invarianten sollten ihre eigene Parameter-Zahl ("arity") kennen, nicht nur
# indirekt über ihre parts erfahren können. 

# Notes:
# All parts of an invariant always use all non-counted variables
# -> the arity of all predicates covered by an invariant is either the
# number of the invariant variables or this value + 1
#
# we currently keep the assumption that each predicate occurs at most once
# in every invariant.

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
    need_more_variables = invariant.arity()
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
        assert len(parts)
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
        # identify equivalence classes
        eq_classes = {}
        for (v1, v2) in self.equalities:
            c1 = eq_classes.setdefault(v1, set([v1]))
            c2 = eq_classes.setdefault(v2, set([v2]))
            if c1 is not c2:
                if len(c2) > len(c1):
                    v1, c1, v2, c2 = v2, c2, v1, c1
                c1.update(c2)
                for elem in c2:
                    eq_classes[elem] = c1

        # create mapping: each key is mapped to the smallest
        # element in its equivalence class (with objects being
        # smaller than variables)
        mapping = {}
        for eq_class in eq_classes.itervalues():
            variables = [item for item in eq_class if item.startswith("?")]
            constants = [item for item in eq_class if not item.startswith("?")]
            if len(constants) >= 2:
                return None # inconsistent assignment (obj1 = obj2)
            if constants:
                set_val = constants[0]
            else:
                set_val = min(variables)
            for entry in eq_class:
                mapping[entry] = set_val
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
    def arity(self):
        return len(self.order)
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
    def arity(self):
        return iter(self.parts).next().arity()
    def get_parameters(self, atom):
        return self.predicate_to_part[atom.predicate].get_parameters(atom)
    def instantiate(self, parameters):
        return [part.instantiate(parameters) for part in self.parts]
    def get_covering_assignments(self, parameters, atom):
        part = self.predicate_to_part[atom.predicate]
        return [part.get_assignment(parameters, atom)]
        # if there were more parts for the same predicate the list
        # contained more than one element
    def check_balance(self, balance_checker, enqueue_func):
        # Check balance for this hypothesis.
        actions_to_check = set()
        heavy_actions_to_check = set()
        for part in self.parts:
            actions_to_check |= balance_checker.get_threats(part.predicate)
        for action in actions_to_check:
            heavy_action = balance_checker.get_heavy_action(action.name)
            if self.operator_too_heavy(heavy_action):
                return False
            if self.operator_unbalanced(action, enqueue_func):
                return False
        return True
    def operator_too_heavy(self, h_action):
        add_effects = [eff for eff in h_action.effects 
                           if not eff.literal.negated and
                              self.predicate_to_part.get(eff.literal.predicate)]
        inv_vars = find_unique_variables(h_action, self)
       
        for eff1, eff2 in itertools.combinations(add_effects, 2):
            negative_clauses = []
            combinatorial_assignments = []
            
            # eff1.atom != eff2.atom
            if (eff1.literal.predicate == eff2.literal.predicate and
                eff1.literal.parts):
                parts = zip(eff1.literal.parts, eff2.literal.parts)
                negative_clauses.append(NegativeClause(parts))
            
            # covers(V, Phi, eff1.atom)
            a = self.get_covering_assignments(inv_vars, eff1.literal)
            combinatorial_assignments.append(a)
            
            # covers(V, Phi, eff2.atom)
            a = self.get_covering_assignments(inv_vars, eff2.literal)
            combinatorial_assignments.append(a)

            # precondition plus effect conditions plus both (negated) literals
            # should be unsatisfiable
            pos = defaultdict(set)
            neg = defaultdict(set)
            neg[eff1.literal.predicate].add(eff1.literal)
            neg[eff2.literal.predicate].add(eff2.literal)
            for literal in itertools.chain(get_literals(h_action.precondition),
                                           get_literals(eff1.condition),
                                           get_literals(eff2.condition)):
                
                if literal.predicate == "=": # use (in)equalities in conditions
                    if literal.negated:
                        n = NegativeClause([literal.args])
                        negative_clauses.append(n)
                    else:
                        a = Assignment([literal.args])
                        combinatorial_assignments.append(a)
                else:
                    if literal.negated:
                        neg[literal.predicate].add(literal)
                    else:
                        pos[literal.predicate].add(literal)

            for pred, posatoms in pos.iteritems():
                if pred in neg:
                    for posatom in posatoms:
                        for negatom in neg[pred]:
                            parts = zip(negatom.args, posatom.args)
                            if parts:
                                negative_clauses.append(NegativeClause(parts))

            # check for all covering assignments whether they make the
            # conjunction of all negative_clauses unsatisfiably
            for assignments in itertools.product(*combinatorial_assignments):
                new_equalities = []
                for a in assignments:
                    new_equalities.extend(a.equalities)
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
        assigs = self.get_covering_assignments(inv_vars, add_effect.literal)

        minimal_renamings = []
        params = [p.name for p in action.parameters]
        for assignment in assigs:
            # renaming of operator parameters must be minimal
            minimality_clauses = []
            mapping = assignment.get_mapping()
            for (n1, n2) in itertools.combinations(params, 2):
                if mapping.get(n1, n1) != mapping.get(n2, n2):
                    minimality_clauses.append(NegativeClause([(n1, n2)]))
            minimal_renamings.append((assignment, minimality_clauses))
       
        lhs_by_pred = defaultdict(list)
        for lit in itertools.chain(get_literals(action.precondition),
                                   get_literals(add_effect.condition),
                                   get_literals(add_effect.literal.negate())):
            lhs_by_pred[lit.predicate].append(lit)

        for del_effect in del_effects:
            if self.del_effect_balances_add_effect(del_effect, add_effect,
                minimal_renamings, inv_vars, lhs_by_pred):
                return False
        
        # Otherwise, no match => Generate new candidates.
        part = self.predicate_to_part[add_effect.literal.predicate]
        for del_eff in [eff for eff in action.effects if eff.literal.negated]:
            if del_eff.literal.predicate not in self.predicate_to_part:
                for match in part.possible_matches(add_effect.literal, del_eff.literal):
                    enqueue_func(Invariant(self.parts.union((match,))))
        return True # balance check fails
    def del_effect_balances_add_effect(self, del_effect, add_effect,
        minimal_renamings, inv_vars, lhs_by_pred):
        negative_clauses = []
        combinatorial_assignments = []

        # add_eff.atom != del_eff.atom
        if (add_effect.literal.predicate == del_effect.literal.predicate and
            add_effect.literal.parts):
            parts = zip(add_effect.literal.parts, del_effect.literal.parts)
            negative_clauses.append(NegativeClause(parts))
        
        # del_effect.atom must be covered
        a = self.get_covering_assignments(inv_vars, del_effect.literal)
        combinatorial_assignments.append(a)
        
        # del_effect.cond and del_effect.atom must be implied by lhs
        for literal in itertools.chain(get_literals(del_effect.condition),
                                       [del_effect.literal.negate()]):
            poss_assignments = []
            for match in lhs_by_pred[literal.predicate]:
                if match.negated != literal.negated:
                    continue
                else:
                    a = Assignment(zip(literal.args, match.args))
                    poss_assignments.append(a)
            if not poss_assignments:
                return False
            combinatorial_assignments.append(poss_assignments)

        for op_param_assignment, minimality_clauses in minimal_renamings:
            # check all promising renamings of the quantified effect variables
            found_renaming = False
            for assignments in itertools.product(*combinatorial_assignments):
                new_equalities = list(op_param_assignment.equalities)
                for a in assignments:
                    new_equalities.extend(a.equalities)
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
                    break
            if not found_renaming:
                return False
        return True
