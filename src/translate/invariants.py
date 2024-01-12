from collections import defaultdict
import itertools

import constraints
import pddl
import tools

# Notes:
# All parts of an invariant always use all non-counted variables
# -> the arity of all predicates covered by an invariant is either the
# number of the invariant variables or this value + 1
#
# we currently keep the assumption that each predicate occurs at most once
# in every invariant.

def invert_list(alist):
    result = defaultdict(list)
    for pos, arg in enumerate(alist):
        result[arg].append(pos)
    return result


def instantiate_factored_mapping(pairs):
    """Input pairs is a list [(preimg1, img1), ..., (preimgn, imgn)].
       For entry (preimg, img), preimg and img is a list of numbers of the same
       length. All preimages (and all imgage) are pairwise disjoint, as well as
       the components of each preimage/image.

       The function determines all possible bijections between the union of
       preimgs and the union of imgs such that for every entry (preimg, img),
       values from preimg are mapped to values from img.
       It yields one permutation after the other, each represented as a list
       of pairs (x,y), meaning x is mapped to y.
       """
    # for every entry (preimg, img) in pairs, determine all possible bijections
    # from preimg to img.
    part_mappings = [[list(zip(preimg, perm_img)) for perm_img in itertools.permutations(img)]
                     for (preimg, img) in pairs]
    # all possibilities to pick one bijection for each entry
    return tools.cartesian_product(part_mappings)
    # TODO: this has a TODO note in tools. It probably can simply be entirely
    # replaced by using itertools.product and chain as follows:
    # for x in itertools.product(*part_mapping):
    #     for y in itertools.chain(*x):
    #         yield list(y)


def find_unique_variables(action, invariant):
    # find unique names for invariant variables
    params = {p.name for p in action.parameters}
    for eff in action.effects:
        params.update([p.name for p in eff.parameters])
    inv_vars = []
    counter = itertools.count()
    for _ in range(invariant.arity()):
        while True:
            new_name = "?v%i" % next(counter)
            if new_name not in params:
                inv_vars.append(new_name)
                break
    return inv_vars


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

       - for (not (= x y)): x != y (as a NegativeClause with one entry(x,y)),
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
                n = constraints.NegativeClause([literal.args])
                system.add_negative_clause(n)
            else:
                a = constraints.Assignment([literal.args])
                # TODO an assignment x=y expects x to be a variable, not an
                # object. Do we need to handle this here?
                system.add_assignment_disjunction([a])
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
                        negative_clause = constraints.NegativeClause(parts)
                        system.add_negative_clause(negative_clause)


def ensure_cover(system, literal, invariant, inv_vars):
    """Modifies the constraint system such that it is only solvable if the
       invariant covers the literal"""
    a = invariant.get_covering_assignments(inv_vars, literal)
    assert len(a) == 1
    # if invariants could contain several parts of one predicate, this
    # assertion would not be true but the depending code in parts relies on
    # this assumption.
    system.add_assignment_disjunction(a)


def ensure_inequality(system, literal1, literal2):
    """Modifies the constraint system such that it is only solvable if the
       literal instantiations are not equal (ignoring whether one is negated and
       the other is not).

       If the literals have different predicates, there is nothing to do.
       Otherwise we add for P(x, y, z) and P(a, b, c) a contraint
       (x != a or y != b or z != c)"""
    if (literal1.predicate == literal2.predicate and
        literal1.args):
        parts = list(zip(literal1.args, literal2.args))
        system.add_negative_clause(constraints.NegativeClause(parts))


class InvariantPart:
    def __init__(self, predicate, order, omitted_pos=-1):
        """There is one InvariantPart for every predicate mentioned in the
           invariant. The parameters of the invariant do not have fixed names
           in the code (to avoid conflicts with existing variable names in
           operators), instead we think of the 0th, 1st, 2nd, ...  parameter.
           The order of the invariant part ties these parameters to the
           arguments of the predicate: the i-th number n_i in order specifies
           that the i-th invariant parameter occurs at position n_i as an
           argument of the predicate. The omitted_pos is the remaining argument
           position, or -1 if there is none."""
        self.predicate = predicate
        self.order = order
        self.omitted_pos = omitted_pos

    def __eq__(self, other):
        # This implies equality of the omitted_pos component.
        return self.predicate == other.predicate and self.order == other.order

    def __ne__(self, other):
        return self.predicate != other.predicate or self.order != other.order

    def __le__(self, other):
        return self.predicate <= other.predicate or self.order <= other.order

    def __lt__(self, other):
        return self.predicate < other.predicate or self.order < other.order

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
        """Returns an assignment constraint that requires the invariant
           parameters to correspond to the values in which they occur in the
           literal."""
        equalities = [(arg, literal.args[argpos])
                      for arg, argpos in zip(parameters, self.order)]
        return constraints.Assignment(equalities)

    def get_parameters(self, literal):
        """Returns the values of the invariant parameters in the literal."""
        return [literal.args[pos] for pos in self.order]

    def instantiate(self, parameters):
        args = ["?X"] * (len(self.order) + (self.omitted_pos != -1))
        for arg, argpos in zip(parameters, self.order):
            args[argpos] = arg
        return pddl.Atom(self.predicate, args)

    def possible_mappings(self, own_literal, other_literal):
        """This method is used when an action had an unbalanced add effect
           own_literal. The action has a delete effect on literal
           other_literal, so we try to refine the invariant such that it also
           covers the delete effect.

           For this purpose, we need to identify all bijections m between
           argument positions of of other_literal and argument positions of
           own_literal, such that other_literal can balance own_literal
           (possibly omitting one argument position of own_literal).

           In most cases this is straight-forward, e.g. with own_literal being
           P(?a, ?b, ?c) and other_literal being Q(?b, ?a, ?c), we would need
           to map the first position of Q to the second position of P, the
           second one of Q to the first one of P, and the third to the third
           one (with 0-indexing represented as [(0,1), (1,0), (2,2)]).

           Most complication stems from the fact that in rare cases arguments
           in the literals can be repeated, e.g. P(?a, ?b, ?a) and Q(?b, ?a,
           ?a). In invariants, we don't have repetitions, so we need to
           identify the more general invariants for which this repetition is
           a special case (if invariant parameters are equal in its
           instantiation). So in this example, we need to consider two
           possible mappings: [(0, 1), (1, 0), (2, 2)] and [(0, 1), (1, 2), (2,
           1)].

           The method returns [] if there is no possible mapping and otherwise
           yields the mappings one by one.
           """
        allowed_omissions = len(other_literal.args) - len(self.order)
        # All parts of an invariant always use all non-counted variables, so
        # len(self.order) corresponds to the number of invariant parameters
        # (the arity of the invariant). So we can/must omit allowed_omissions
        # many arguments of other_literal when matching invariant parameters
        # with arguments.
        if allowed_omissions not in (0, 1):
            # There may be at most one counted variable.
            return []
        own_parameters = self.get_parameters(own_literal)
        # own_parameters is a list containing at position i the value of the
        # i-th invariant parameter in own_literal.
        arg_to_ordered_pos = invert_list(own_parameters)
        other_arg_to_pos = invert_list(other_literal.args)
        # other_arg_to_pos maps every argument of other_literal to the
        # lists of positions in which it is occuring in other_literal, e.g.
        # for P(?a, ?b, ?a), other_arg_to_pos["?a"] = [0, 2].
        factored_mapping = []

        # We iterate over all values occuring as arguments in other_literal
        # and compare the number of occurrences in other_literal to those in
        # own_literal. If the difference of these numbers allows us to cover
        # other_literal with the (still) permitted number of counted variables,
        # we remember the correspondance of positions for this value in
        # factored_mapping. If all values can be covered, we instatiate the
        # complete factored_mapping, computing all possibilities to map
        # positions from own_literal to positions from other_literal.
        for key, other_positions in other_arg_to_pos.items():
            own_positions = arg_to_ordered_pos.get(key, [])
            # all positions at which key occurs as argument in own_literal
            len_diff = len(own_positions) - len(other_positions)
            if len_diff >= 1 or len_diff <= -2 or len_diff == -1 and not allowed_omissions:
                # mapping of the literals is not possible with at most one
                # counted variable.
                return []
            if len_diff:
                own_positions.append(-1)
                allowed_omissions = 0
            factored_mapping.append((other_positions, own_positions))
        return instantiate_factored_mapping(factored_mapping)

    def possible_matches(self, own_literal, other_literal):
        """This method is used when an action had an unbalanced add effect
           own_literal. The action has a delete effect on literal
           other_literal, so we try to refine the invariant such that it also
           covers the delete effect.

           For this purpose, we consider all possible mappings from the
           parameter positions of other_literal to the parameter positions of
           own_literal such that the extended invariant can use other_literal
           to balance own_literal. From these position mapping, we can extract
           the order field for the invariant part.

           Consider for an example of the "self" InvariantPart "forall ?u,
           ?v, ?w(P(?u, ?v, ?w) is non-increasing" and let own_literal be P(?a, ?b,
           ?c) and other_literal be Q(?b, ?c, ?d, ?a)). The only possible mapping
           from positions of Q to positions of P (or -1) is [0->1, 1->2, 2->-1,
           3->0] for which we create a new Invariant Part Q(?v, ?w, _. ?u) with
           the third argument being counted.
        """
        assert self.predicate == own_literal.predicate
        result = []
        for mapping in self.possible_mappings(own_literal, other_literal):
            new_order = [None] * len(self.order)
            omitted = -1
            for (other_pos, own_pos) in mapping:
                if own_pos == -1:
                    omitted = other_pos
                else:
                    new_order[own_pos] = other_pos
                    # TODO This only works because in the initial invariant
                    # candidates the numbering of invariant parameters matches
                    # the order of the corresponding arguments in the
                    # predicate. To make this work in general, we need to
                    # consider a potential order difference in self.order.
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
        self.predicates = {part.predicate for part in parts}
        self.predicate_to_part = {part.predicate: part for part in parts}
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
        return "{%s}" % ", ".join(str(part) for part in self.parts)

    def __repr__(self):
        return '<Invariant %s>' % self

    def arity(self):
        return next(iter(self.parts)).arity()

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
        for part in self.parts:
            actions_to_check |= balance_checker.get_threats(part.predicate)
        for action in sorted(actions_to_check, key=lambda a: (a.name,
                                                              a.parameters)):
            heavy_action = balance_checker.get_heavy_action(action)
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

        if len(add_effects) <= 1:
            return False

        for eff1, eff2 in itertools.combinations(add_effects, 2):
            system = constraints.ConstraintSystem()
            ensure_inequality(system, eff1.literal, eff2.literal)
            ensure_cover(system, eff1.literal, self, inv_vars)
            ensure_cover(system, eff2.literal, self, inv_vars)
            ensure_conjunction_sat(system, get_literals(h_action.precondition),
                                   get_literals(eff1.condition),
                                   get_literals(eff2.condition),
                                   [eff1.literal.negate()],
                                   [eff2.literal.negate()])
            if system.is_solvable():
                return True
        return False

    def operator_unbalanced(self, action, enqueue_func):
        inv_vars = find_unique_variables(action, self)
        relevant_effs = [eff for eff in action.effects
                         if self.predicate_to_part.get(eff.literal.predicate)]
        add_effects = [eff for eff in relevant_effs
                       if not eff.literal.negated]
        del_effects = [eff for eff in relevant_effs
                       if eff.literal.negated]
        for eff in add_effects:
            if self.add_effect_unbalanced(action, eff, del_effects, inv_vars,
                                          enqueue_func):
                return True
        return False

    def minimal_covering_renamings(self, action, add_effect, inv_vars):
        """computes the minimal renamings of the action parameters such
           that the add effect is covered by the action.
           Each renaming is an constraint system"""

        # add_effect must be covered
        assigs = self.get_covering_assignments(inv_vars, add_effect.literal)

        # renaming of operator parameters must be minimal
        minimal_renamings = []
        params = [p.name for p in action.parameters]
        for assignment in assigs:
            system = constraints.ConstraintSystem()
            system.add_assignment(assignment)
            mapping = assignment.get_mapping()
            if len(params) > 1:
                for (n1, n2) in itertools.combinations(params, 2):
                    if mapping.get(n1, n1) != mapping.get(n2, n2):
                        negative_clause = constraints.NegativeClause([(n1, n2)])
                        system.add_negative_clause(negative_clause)
            minimal_renamings.append(system)
        return minimal_renamings

    def add_effect_unbalanced(self, action, add_effect, del_effects,
                              inv_vars, enqueue_func):

        minimal_renamings = self.minimal_covering_renamings(action, add_effect,
                                                            inv_vars)

        lhs_by_pred = defaultdict(list)
        for lit in itertools.chain(get_literals(action.precondition),
                                   get_literals(add_effect.condition),
                                   get_literals(add_effect.literal.negate())):
            lhs_by_pred[lit.predicate].append(lit)

        for del_effect in del_effects:
            minimal_renamings = self.unbalanced_renamings(
                del_effect, add_effect, inv_vars, lhs_by_pred, minimal_renamings)
            if not minimal_renamings:
                return False

        # Otherwise, the balance check fails => Generate new candidates.
        self.refine_candidate(add_effect, action, enqueue_func)
        return True

    def refine_candidate(self, add_effect, action, enqueue_func):
        """refines the candidate for an add effect that is unbalanced in the
           action and adds the refined one to the queue"""
        part = self.predicate_to_part[add_effect.literal.predicate]
        for del_eff in [eff for eff in action.effects if eff.literal.negated]:
            if del_eff.literal.predicate not in self.predicate_to_part:
                for match in part.possible_matches(add_effect.literal,
                                                   del_eff.literal):
                    enqueue_func(Invariant(self.parts.union((match,))))

    def unbalanced_renamings(self, del_effect, add_effect, inv_vars,
                             lhs_by_pred, unbalanced_renamings):
        """returns the renamings from unbalanced renamings for which
           the del_effect does not balance the add_effect."""

        system = constraints.ConstraintSystem()
        ensure_cover(system, del_effect.literal, self, inv_vars)

        # Since we may only rename the quantified variables of the delete effect
        # we need to check that "renamings" of constants are already implied by
        # the unbalanced_renaming (of the of the operator parameters). The
        # following system is used as a helper for this. It builds a conjunction
        # that formulates that the constants are NOT renamed accordingly. We
        # below check that this is impossible with each unbalanced renaming.
        check_constants = False
        constant_test_system = constraints.ConstraintSystem()
        for a, b in system.combinatorial_assignments[0][0].equalities:
            # first 0 because the system was empty before we called ensure_cover
            # second 0 because ensure_cover only adds assignments with one entry
            if b[0] != "?":
                check_constants = True
                neg_clause = constraints.NegativeClause([(a, b)])
                constant_test_system.add_negative_clause(neg_clause)

        ensure_inequality(system, add_effect.literal, del_effect.literal)

        still_unbalanced = []
        for renaming in unbalanced_renamings:
            if check_constants:
                new_sys = constant_test_system.combine(renaming)
                if new_sys.is_solvable():
                    # it is possible that the operator arguments are not
                    # mapped to constants as required for covering the delete
                    # effect
                    still_unbalanced.append(renaming)
                    continue

            new_sys = system.combine(renaming)
            if self.lhs_satisfiable(renaming, lhs_by_pred):
                implies_system = self.imply_del_effect(del_effect, lhs_by_pred)
                if not implies_system:
                    still_unbalanced.append(renaming)
                    continue
                new_sys = new_sys.combine(implies_system)
            if not new_sys.is_solvable():
                still_unbalanced.append(renaming)
        return still_unbalanced

    def lhs_satisfiable(self, renaming, lhs_by_pred):
        system = renaming.copy()
        ensure_conjunction_sat(system, *itertools.chain(lhs_by_pred.values()))
        return system.is_solvable()

    def imply_del_effect(self, del_effect, lhs_by_pred):
        """returns a constraint system that is solvable if lhs implies
           the del effect (only if lhs is satisfiable). If a solvable
           lhs never implies the del effect, return None."""
        # del_effect.cond and del_effect.atom must be implied by lhs
        implies_system = constraints.ConstraintSystem()
        for literal in itertools.chain(get_literals(del_effect.condition),
                                       [del_effect.literal.negate()]):
            poss_assignments = []
            for match in lhs_by_pred[literal.predicate]:
                if match.negated != literal.negated:
                    continue
                else:
                    a = constraints.Assignment(list(zip(literal.args, match.args)))
                    poss_assignments.append(a)
            if not poss_assignments:
                return None
            implies_system.add_assignment_disjunction(poss_assignments)
        return implies_system
