from collections import defaultdict
import itertools
import logging

import constraints
import pddl
import tools

# logging.basicConfig(level=logging.DEBUG)
# Notes:
# All parts of an invariant always use all non-counted variables
# -> the arity of all predicates covered by an invariant is either the
# number of the invariant variables or this value + 1
#
# we currently keep the assumption that each predicate occurs at most once
# in every invariant.


def instantiate_factored_mapping(pairs):
    """Input pairs is a list [(preimg1, img1), ..., (preimgn, imgn)].
       For entry (preimg, img), preimg is a list of numbers and img a list of
       invariant parameters or -1 of the same length. All preimages (and all
       images) are pairwise disjoint, as well as the components of each
       preimage/image.

       The function determines all possible bijections between the union of
       preimgs and the union of imgs such that for every entry (preimg, img),
       all values from preimg are mapped to values from img.
       It yields one permutation after the other, each represented as a list
       of pairs (x,y), meaning x is mapped to y.
       """
    # for every entry (preimg, img) in pairs, determine all possible bijections
    # from preimg to img.
    part_mappings = [[list(zip(preimg, perm_img)) for perm_img in itertools.permutations(img)]
                     for (preimg, img) in pairs]
    # all possibilities to pick one bijection for each entry
    if not part_mappings:
        yield []
    else:
        for x in itertools.product(*part_mappings):
            yield list(itertools.chain.from_iterable(x))


def get_literals(condition):
    if isinstance(condition, pddl.Literal):
        yield condition
    elif isinstance(condition, pddl.Conjunction):
        yield from condition.parts


def ensure_conjunction_sat(system, *parts):
    """Modifies the constraint system such that it is only solvable if the
       conjunction of all parts is satisfiable.

       Each part must be an iterator, generator, or an iterable over
       literals.

       We add the following constraints for each literal to the system:

       - for (not (= x y)): x != y (as an InequalityDisjunction with one entry
         (x,y)),
       - for (= x y): x = y
       - for predicates that occur with a positive and negative literal, we
         consider every combination of a positive one (e.g. P(x, y, z)) and
         a negative one (e.g. (not P(a, b, c))) and add a constraint
         (x != y or y != b or z != c)."""
    pos = defaultdict(set)
    neg = defaultdict(set)
    for literal in itertools.chain(*parts):
        if literal.predicate == "=": # use (in)equalities in conditions
            if literal.negated:
                d = constraints.InequalityDisjunction([literal.args])
                system.add_inequality_disjunction(d)
            else:
                a = constraints.EqualityConjunction([literal.args])
                # TODO an assignment x=y expects x to be a variable, not an
                # object. Do we need to handle this here?
                print("Hier?", type(a))
                system.add_equality_DNF([a])
        else:
            if literal.negated:
                neg[literal.predicate].add(literal)
            else:
                pos[literal.predicate].add(literal)

    for pred, posatoms in pos.items():
        if pred in neg:
            for posatom in posatoms:
                for negatom in neg[pred]:
                    parts = list(zip(negatom.args, posatom.args))
                    if parts:
                        ineq_disj = constraints.InequalityDisjunction(parts)
                        system.add_inequality_disjunction(ineq_disj)


def ensure_cover(system, literal, invariant):
    """Modifies the constraint system such that it is only solvable if the
       invariant covers the literal."""
    a = invariant.get_covering_assignment(literal)
    system.add_equality_DNF([a])


def ensure_inequality(system, literal1, literal2):
    """Modifies the constraint system such that it is only solvable if the
       literal instantiations are not equal (ignoring whether one is negated and
       the other is not).

       If the literals have different predicates, there is nothing to do.
       Otherwise we add for P(x, y, z) and P(a, b, c) a contraint
       (x != a or y != b or z != c)."""
    if (literal1.predicate == literal2.predicate and literal1.args):
        parts = list(zip(literal1.args, literal2.args))
        system.add_inequality_disjunction(constraints.InequalityDisjunction(parts))


class InvariantPart:
    def __init__(self, atom, omitted_pos=-1):
        """There is one InvariantPart for every predicate mentioned in the
           invariant. The atom of the invariant part has arguments of the form
           "?@vi" for the invariant parameters and "_" at the omitted position.
           If no position is omitted, omitted_pos is -1."""
        self.atom = atom
        self.omitted_pos = omitted_pos

    def __eq__(self, other):
        # This implies equality of the omitted_pos component.
        return self.atom == other.atom

    def __ne__(self, other):
        return self.atom != other.atom

    def __le__(self, other):
        return self.atom <= other.atom

    def __lt__(self, other):
        return self.atom < other.atom

    def __hash__(self):
        return hash(self.atom)

    def __str__(self):
        return f"{self.atom} [omitted_pos = {self.omitted_pos}]"

    def arity(self):
        if self.omitted_pos == -1:
            return len(self.atom.args)
        else:
            return len(self.atom.args) - 1

    def get_parameters(self, literal):
        """Returns a dictionary, mapping the invariant parameters to the
           corresponding values in the literal."""
        return dict((arg, literal.args[pos])
                    for pos, arg in enumerate(self.atom.args)
                    if pos != self.omitted_pos)

    def instantiate(self, parameters_tuple):
        parameters = dict(parameters_tuple)
        args = [parameters[arg] if arg != "_" else "?X"
                for arg in self.atom.args]
        return pddl.Atom(self.atom.predicate, args)

    def possible_mappings(self, own_literal, other_literal):
        """This method is used when an action had an unbalanced add effect
           own_literal. The action has a delete effect on literal
           other_literal, so we try to refine the invariant such that it also
           covers the delete effect.

           From own_literal, we can determine a variable or object for every
           invariant parameter, where potentially several invariant parameters
           can have the same value.

           From the arguments of other_literal, we determine all possibilities
           how we can use the invariant parameters as arguments of
           other_literal so that the values match (possibly covering one
           parameter with a placeholder/counted variable). Since there also can
           be duplicates in the argumets of other_literal, we cannot operate on
           the arguments directly, but instead operate on the positions.

           The method returns [] if there is no possible mapping and otherwise
           yields the mappings from the positions of other to the invariant
           variables or -1 one by one.
           """
        allowed_omissions = len(other_literal.args) - self.arity()
        # All parts of an invariant always use all non-counted variables, of
        # which we have arity many. So we must omit allowed_omissions many
        # arguments of other_literal when matching invariant parameters with
        # arguments.
        if allowed_omissions not in (0, 1):
            # There may be at most one counted variable.
            return []
        own_parameters = self.get_parameters(own_literal)
        # own_parameters is a dictionary mapping the invariant parameters to
        # the corresponding parameter of own_literal
        ownarg_to_invariant_parameters = defaultdict(list)
        for x, y in own_parameters.items():
            ownarg_to_invariant_parameters[y].append(x)

        # other_arg_to_pos maps every argument of other_literal to the
        # lists of positions in which it is occuring in other_literal, e.g.
        # for P(?a, ?b, ?a), other_arg_to_pos["?a"] = [0, 2].
        other_arg_to_pos = defaultdict(list)
        for pos, arg in enumerate(other_literal.args):
            other_arg_to_pos[arg].append(pos)

        factored_mapping = []

        # We iterate over all values occuring as arguments in other_literal
        # and compare the number of occurrences in other_literal to those in
        # own_literal. If the difference of these numbers allows us to cover
        # other_literal with the (still) permitted number of counted variables,
        # we store the correspondance of all argument positions of
        # other_literal for this value to the invariant parameters at these
        # positions in factored_mapping. If all values can be covered, we
        # instatiate the complete factored_mapping, computing all possibilities
        # to map positions from other_literal to invariant parameters (or -1 if
        # the position is omitted).
        for key, other_positions in other_arg_to_pos.items():
            inv_params = ownarg_to_invariant_parameters[key]
            # all positions at which key occurs as argument in own_literal
            len_diff = len(inv_params) - len(other_positions)
            if len_diff >= 1 or len_diff <= -2 or len_diff == -1 and not allowed_omissions:
                # mapping of the literals is not possible with at most one
                # counted variable.
                return []
            if len_diff:
                inv_params.append(-1)
                allowed_omissions = 0
            factored_mapping.append((other_positions, inv_params))
        return instantiate_factored_mapping(factored_mapping)

    def possible_matches(self, own_literal, other_literal):
        """This method is used when an action had an unbalanced add effect
           on own_literal. The action has a delete effect on literal
           other_literal, so we try to refine the invariant such that it also
           covers the delete effect.

           For this purpose, we consider all possible mappings from the
           parameter positions of other_literal to the parameter positions of
           own_literal such that the extended invariant can use other_literal
           to balance own_literal. From these position mapping, we can extract
           the new invariant part.

           Consider for an example of the "self" InvariantPart "forall ?@v0,
           ?@v1, ?@v2 P(?@v0, ?@v1, ?@v2) is non-increasing" and let
           own_literal be P(?a, ?b, ?c) and other_literal be Q(?b, ?c, ?d, ?a).
           The only possible mapping from positions of Q to invariant variables
           of P (or -1) is [0->?@v1, 1->?@v2, 2->-1, 3->?@v0] for which we
           create a new Invariant Part Q(?@v1, ?@v2, _. ?@v0) with the third
           argument being counted.
        """
        assert self.atom.predicate == own_literal.predicate
        result = []
        for mapping in self.possible_mappings(own_literal, other_literal):
            args = ["_"] * len(other_literal.args)
            omitted = -1
            for (other_pos, inv_var) in mapping:
                if inv_var == -1:
                    omitted = other_pos
                else:
                    args[other_pos] = inv_var
            atom = pddl.Atom(other_literal.predicate, args)
            result.append(InvariantPart(atom, omitted))
        return result


class Invariant:
    # An invariant is a logical expression of the type
    #   forall ?@v1...?@vk: sum_(part in parts) weight(part, ?@v1, ..., ?@vk) <= 1.
    # k is called the arity of the invariant.
    # A "part" is an atom that only contains arguments from {?@v1, ..., ?@vk, _};
    # the symbol _ may occur at most once.

    def __init__(self, parts):
        self.parts = frozenset(parts)
        self.predicates = {part.atom.predicate for part in parts}
        self.predicate_to_part = {part.atom.predicate: part for part in parts}
        assert len(self.parts) == len(self.predicates)

    def __eq__(self, other):
        return self.parts == other.parts

    def __ne__(self, other):
        return self.parts != other.parts

    def __lt__(self, other):
        return self.parts < other.parts

    def __le__(self, other):
        return self.parts <= other.parts

    def __hash__(self):
        return hash(self.parts)

    def __str__(self):
        return "{%s}" % ", ".join(sorted(str(part) for part in self.parts))

    def __repr__(self):
        return '<Invariant %s>' % self

    def arity(self):
        return next(iter(self.parts)).arity()

    def get_parameters(self, atom):
        return self.predicate_to_part[atom.predicate].get_parameters(atom)

    def instantiate(self, parameters):
        return [part.instantiate(parameters) for part in self.parts]

    def get_covering_assignment(self, literal):
        """This is only called for atoms with a predicate for which the
           invariant has a part. It returns an assignment constraint that
           requires that each parameter of the invariant matches the
           corresponding argument of the given literal.

           Example: If the literal is P(?a, ?b, ?c), the invariant part for P
           is P(?@v0, _, ?@v1) then the method returns the constraint (?@v0 = ?a
           and ?@v1 = ?c). It is irrelevant whether the literal is negated.
           """
        part = self.predicate_to_part[literal.predicate]
        equalities = [(inv_param, literal.args[pos])
                      for pos, inv_param in enumerate(part.atom.args)
                      if pos != part.omitted_pos] # alternatively inv_param != "_"
        return constraints.EqualityConjunction(equalities)
        # If there were more parts for the same predicate, we would have to
        # consider more than one assignment (disjunctively).
        # We assert earlier that this is not the case.

    def check_balance(self, balance_checker, enqueue_func):
        # Check balance for this hypothesis.
        actions_to_check = set()
        for part in self.parts:
            actions_to_check |= balance_checker.get_threats(part.atom.predicate)
        for action in sorted(actions_to_check, key=lambda a: (a.name,
                                                              a.parameters)):
            heavy_action = balance_checker.get_heavy_action(action)
#            logging.debug(f"Checking Invariant {self} wrt action {action.name}")
            if self.operator_too_heavy(heavy_action):
#                logging.debug("too heavy")
                return False
            if self.operator_unbalanced(action, enqueue_func):
                return False
        return True

    def operator_too_heavy(self, h_action):
        add_effects = [eff for eff in h_action.effects
                       if not eff.literal.negated and
                       self.predicate_to_part.get(eff.literal.predicate)]

        if len(add_effects) <= 1:
            return False

        for eff1, eff2 in itertools.combinations(add_effects, 2):
            system = constraints.ConstraintSystem()
            ensure_inequality(system, eff1.literal, eff2.literal)
            ensure_cover(system, eff1.literal, self)
            ensure_cover(system, eff2.literal, self)
            ensure_conjunction_sat(system, get_literals(h_action.precondition),
                                   get_literals(eff1.condition),
                                   get_literals(eff2.condition),
                                   [eff1.literal.negate()],
                                   [eff2.literal.negate()])
            if system.is_solvable():
                return True
        return False

    def operator_unbalanced(self, action, enqueue_func):
#        logging.debug(f"Checking Invariant {self} for action {action.name}")
        relevant_effs = [eff for eff in action.effects
                         if self.predicate_to_part.get(eff.literal.predicate)]
        add_effects = [eff for eff in relevant_effs
                       if not eff.literal.negated]
        del_effects = [eff for eff in relevant_effs
                       if eff.literal.negated]
        for eff in add_effects:
            if self.add_effect_unbalanced(action, eff, del_effects,
                                          enqueue_func):
                return True
#        logging.debug(f"Balanced")
        return False

    def add_effect_unbalanced(self, action, add_effect, del_effects,
                              enqueue_func):
        # We build for every delete effect that is possibly covered by this
        # invariant a constraint system that will be solvable if the delete
        # effect balances the add effect. Large parts of the constraint system
        # are independent of the delete effect, so we precompute them first.

        # Dictionary add_effect_produced_by_pred describes what must be true so
        # that the action is applicable and produces the add effect. It is
        # stored as a map from predicate names to literals (overall
        # representing a conjunction of these literals).
        add_effect_produced_by_pred = defaultdict(list)
        for lit in itertools.chain(get_literals(action.precondition),
                                   get_literals(add_effect.condition),
                                   get_literals(add_effect.literal.negate())):
            add_effect_produced_by_pred[lit.predicate].append(lit)

        # threat_assignment is a conjunction of equalities that sets each
        # invariant parameter equal to its value in add_effect.literal.
        threat_assignment = self.get_covering_assignment(add_effect.literal)

        # The assignment can imply equivalences between variables (and with
        # constants). For example if the invariant part is P(_ ?@v0 ?@v1 ?@v2)
        # and the add effect is P(?x ?y ?y a) then we would know that the
        # invariant part is only threatened by the add effect if the first two
        # invariant parameters are equal and the third parameter is a.

        # The add effect must be balanced in all threatening action
        # applications. We thus must adapt the constraint system such that it
        # prevents restricting solution that set action parameters equal to
        # each other or to a specific constant (if this is not already
        # necessary for the threat).
        constants = set(a for a in add_effect.literal.args if a[0] != "?")
        for del_effect in del_effects:
            for a in del_effect.literal.args:
                if a[0] != "?":
                    constants.add(a)
        params = [p.name for p in action.parameters]
        action_param_system = constraints.ConstraintSystem()
        representative = threat_assignment.get_representative()
        # The assignment is a conjunction of equalities between invariant
        # parameters and the variables and constants occurring in the
        # add_effect literal. Dictionary representative maps every term to its
        # representative in the finest equivalence relation induced by the
        # equalities.
        for param in params:
            if representative.get(param, param)[0] == "?":
                # for the add effect being a threat to the invariant, param
                # does not need to be a specific constant. So we may not bind
                # it to a constant when balancing the add effect.
                for c in constants:
                    ineq_disj = constraints.InequalityDisjunction([(param, c)])
                    action_param_system.add_inequality_disjunction(ineq_disj)
        for (n1, n2) in itertools.combinations(params, 2):
            if representative.get(n1, n1) != representative.get(n2, n2):
                # n1 and n2 don't have to be equivalent to cover the add
                # effect, so we require for the solutions that they do not
                # set n1 and n2 equal.
                ineq_disj = constraints.InequalityDisjunction([(n1, n2)])
                action_param_system.add_inequality_disjunction(ineq_disj)

        for del_effect in del_effects:
            if self.balances(del_effect, add_effect,
                             add_effect_produced_by_pred,
                             threat_assignment, action_param_system):
#                logging.debug(f"Invariant {self}: add effect {add_effect.literal} balanced for action {action.name} by del effect {del_effect.literal}")
                return False

        # The balance check fails => Generate new candidates.
        self.refine_candidate(add_effect, action, enqueue_func)
        return True

    def refine_candidate(self, add_effect, action, enqueue_func):
        """Refines the candidate for an add effect that is unbalanced in the
           action and adds the refined one to the queue."""
        part = self.predicate_to_part[add_effect.literal.predicate]
        for del_eff in [eff for eff in action.effects if eff.literal.negated]:
            if del_eff.literal.predicate not in self.predicate_to_part:
                for match in part.possible_matches(add_effect.literal,
                                                   del_eff.literal):
                    enqueue_func(Invariant(self.parts.union((match,))))

    def balances(self, del_effect, add_effect, produced_by_pred,
                 threat_assignment, action_param_system):
        """Returns whether the del_effect is guaranteed to balance the add effect
           where the input is such that:
           - produced_by_pred must be true for the add_effect to be produced,
           - threat_assignment is a constraint of equalities that must be true
             for the add effect threatening the invariant, and
           - action_param_system contains contraints that action parameters are
             not fixed to be equivalent (except the add effect is otherwise not
             threat)."""

        implies_system = self._imply_consumption(del_effect, produced_by_pred)
        if not implies_system:
            # it is impossible to guarantee that every production by add_effect
            # implies a consumption by del effect.
            return False

        # We will build a system that is solvable if the delete effect is
        # guaranteed to balance the add effect for this invariant.
        system = constraints.ConstraintSystem()
        system.add_equality_conjunction(threat_assignment)
        # invariant parameters equal the corresponding arguments of the add
        # effect atom.

        ensure_cover(system, del_effect.literal, self)
        # invariant parameters equal the corresponding arguments of the add
        # effect atom.

        system.extend(implies_system)
        # possible assignments such that a production by the add
        # effect implies a consumption by the delete effect.

        # So far, the system implicitly represents all possibilities
        # how every production of the add effect has a consumption by the
        # delete effect while both atoms are covered by the invariant. However,
        # we did not yet consider add-after-delete semantics and we include
        # possibilities that restrict the action parameters. To prevent these,
        # we in the following add a number of inequality disjunctions.

        system.extend(action_param_system)
        # prevent restricting assignments of action parameters (must be
        # balanced independent of the concrete action instantiation).

        ensure_inequality(system, add_effect.literal, del_effect.literal)
        # if the add effect and the delete effect affect the same predicate
        # then their arguments must differ in at least one position.

        if not system.is_solvable():
            # The system is solvable if there is a consistenst possibility to
            return False

#        logging.debug(f"{system}")
        return True

    def _imply_consumption(self, del_effect, literals_by_pred):
        """Returns a constraint system that is solvable if the conjunction of
           literals occurring as values in dictionary literals_by_pred implies
           the consumption of the atom of the delete effect. We return None if
           we detect that the constraint system would never be solvable.
           """
        implies_system = constraints.ConstraintSystem()
        for literal in itertools.chain(get_literals(del_effect.condition),
                                       [del_effect.literal.negate()]):
            poss_assignments = []
            for match in literals_by_pred[literal.predicate]:
                if match.negated != literal.negated:
                    continue
                else:
                    # match implies literal iff they agree on each argument
                    a = constraints.EqualityConjunction(list(zip(literal.args, match.args)))
                    poss_assignments.append(a)
            if not poss_assignments:
                return None
            implies_system.add_equality_DNF(poss_assignments)
        return implies_system
