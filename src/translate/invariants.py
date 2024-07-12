from collections import defaultdict
import itertools

import constraints
import pddl
import tools

# Notes:
# All parts of an invariant always use all non-counted variables
# -> the arity of all predicates covered by an invariant is either the
# number of the invariant parameters or this value + 1
#
# We only consider invariants where each predicate occurs in at most one part.

COUNTED = -1

def instantiate_factored_mapping(pairs):
    """Input pairs is a list [(preimg1, img1), ..., (preimgn, imgn)].
       For entry (preimg, img), preimg is a list of numbers and img a list of
       invariant parameters or COUNTED of the same length. All preimages (and
       all images) are pairwise disjoint, as well as the components of each
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
         (x != a or y != b or z != c)."""
    pos = defaultdict(set)
    neg = defaultdict(set)
    for literal in itertools.chain(*parts):
        if literal.predicate == "=": # use (in)equalities in conditions
            if literal.negated:
                d = constraints.InequalityDisjunction([literal.args])
                system.add_inequality_disjunction(d)
            else:
                a = constraints.EqualityConjunction([literal.args])
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
    """Modifies the constraint system such that in every solution the invariant
       covers the literal (= invariant parameters are equivalent to the
       corresponding argument in the literal)."""
    cover = invariant._get_cover_equivalence_conjunction(literal)
    system.add_equality_DNF([cover])


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
    def __init__(self, predicate, args, omitted_pos=None):
        """There is one InvariantPart for every predicate mentioned in the
           invariant. The arguments args contain numbers 0,1,... for the
           invariant parameters and COUNTED at the omitted position.
           If no position is omitted, omitted_pos is None, otherwise it is the
           index of COUNTED in args."""
        self.predicate = predicate
        self.args = tuple(args)
        self.omitted_pos = omitted_pos

    def __eq__(self, other):
        # This implies equality of the omitted_pos component.
        return self.predicate == other.predicate and self.args == other.args

    def __ne__(self, other):
        return self.predicate != other.predicate or self.args != other.args

    def __le__(self, other):
        return (self.predicate, self.args) <= (other.predicate, other.args)

    def __lt__(self, other):
        return (self.predicate, self.args) < (other.predicate, other.args)

    def __hash__(self):
        return hash((self.predicate, self.args))

    def __str__(self):
        return f"{self.predicate}({self.args}) [omitted_pos = {self.omitted_pos}]"

    def arity(self):
        if self.omitted_pos is None:
            return len(self.args)
        else:
            return len(self.args) - 1

    def get_parameters(self, literal):
        """Returns a dictionary, mapping the invariant parameters to the
           corresponding values in the literal."""
        return dict((arg, literal.args[pos])
                    for pos, arg in enumerate(self.args)
                    if pos != self.omitted_pos)

    def instantiate(self, parameters_tuple):
        args = [parameters_tuple[arg] if arg != COUNTED else "?X"
                for arg in self.args]
        return pddl.Atom(self.predicate, args)

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
           variables or COUNTED one by one.
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
        # to map positions from other_literal to invariant parameters (or
        # COUNTED if the position is omitted).
        for key, other_positions in other_arg_to_pos.items():
            inv_params = ownarg_to_invariant_parameters[key]
            # all positions at which key occurs as argument in own_literal
            len_diff = len(inv_params) - len(other_positions)
            if len_diff >= 1 or len_diff <= -2 or len_diff == -1 and not allowed_omissions:
                # mapping of the literals is not possible with at most one
                # counted variable.
                return []
            if len_diff:
                inv_params.append(COUNTED)
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
           of P (or COUNTED) is [0->?@v1, 1->?@v2, 2->COUNTED, 3->?@v0] for
           which we create a new Invariant Part Q(?@v1, ?@v2, _. ?@v0) with the
           third argument being counted.
        """
        assert self.predicate == own_literal.predicate
        for mapping in self.possible_mappings(own_literal, other_literal):
            args = [COUNTED] * len(other_literal.args)
            omitted = None
            for (other_pos, inv_var) in mapping:
                if inv_var == COUNTED:
                    omitted = other_pos
                else:
                    args[other_pos] = inv_var
            yield InvariantPart(other_literal.predicate, args, omitted)


class Invariant:
    # An invariant is a logical expression of the type
    #   forall ?@v1...?@vk: sum_(part in parts) weight(part, ?@v1, ..., ?@vk) <= 1.
    # k is called the arity of the invariant.
    # A "part" is an atom that only contains arguments from {?@v1, ..., ?@vk,
    # COUNTED} but instead of ?@vi, we store it as int i; COUNTED may occur at
    # most once.

    def __init__(self, parts):
        self.parts = frozenset(parts)
        self.predicate_to_part = {part.predicate: part for part in parts}
        self.predicates = set(self.predicate_to_part.keys())
        assert len(self.parts) == len(self.predicates)

    def __eq__(self, other):
        return self.parts == other.parts

    def __ne__(self, other):
        return self.parts != other.parts

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

    def _get_cover_equivalence_conjunction(self, literal):
        """This is only called for atoms with a predicate for which the
           invariant has a part. It returns an equivalence conjunction that
           requires every invariant parameter to be equal to the corresponding
           argument of the given literal. For the result, we do not consider
           whether the literal is negated.

           Example: If the literal is P(?a, ?b, ?c), the invariant part for P
           is P(?@v0, _, ?@v1) then the method returns the constraint (?@v0 = ?a
           and ?@v1 = ?c).
           """
        part = self.predicate_to_part[literal.predicate]
        equalities = [(arg, literal.args[pos])
                      for pos, arg in enumerate(part.args)
                      if arg != COUNTED]
        return constraints.EqualityConjunction(equalities)
        # If there were more parts for the same predicate, we would have to
        # consider more than one assignment (disjunctively).
        # We assert earlier that this is not the case.

    def check_balance(self, balance_checker, enqueue_func):
        # Check balance for this hypothesis.
        actions_to_check = dict()
        # We will only use the keys of the dictionary. We do not use a set
        # because it's not stable and introduces non-determinism in the
        # invariance analysis.
        for part in sorted(self.parts):
            for a in balance_checker.get_threats(part.predicate):
                actions_to_check[a] = True

        actions = list(actions_to_check.keys())
        while actions:
            # For a better expected perfomance, we want to randomize the order
            # in which actions are checked. Since candidates are often already
            # discarded by an early check, we do not want to shuffle the order
            # but instead always draw the next action randomly from those we
            # did not yet consider.
            pos = balance_checker.random.randrange(len(actions))
            actions[pos], actions[-1] = actions[-1], actions[pos]
            action = actions.pop()
            heavy_action = balance_checker.get_heavy_action(action)
            if self._operator_too_heavy(heavy_action):
                return False
            if self._operator_unbalanced(action, enqueue_func):
                return False
        return True

    def _operator_too_heavy(self, h_action):
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

    def _operator_unbalanced(self, action, enqueue_func):
        relevant_effs = [eff for eff in action.effects
                         if self.predicate_to_part.get(eff.literal.predicate)]
        add_effects = [eff for eff in relevant_effs
                       if not eff.literal.negated]
        del_effects = [eff for eff in relevant_effs
                       if eff.literal.negated]
        for eff in add_effects:
            if self._add_effect_unbalanced(action, eff, del_effects,
                                           enqueue_func):
                return True
        return False

    def _add_effect_unbalanced(self, action, add_effect, del_effects,
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

        # add_cover is an equality conjunction that sets each invariant
        # parameter equal to its value in add_effect.literal.
        add_cover = self._get_cover_equivalence_conjunction(add_effect.literal)

        # add_cover can imply equivalences between variables (and with
        # constants). For example if the invariant part is P(_ ?@v0 ?@v1 ?@v2)
        # and the add effect is P(?x ?y ?y a) then we would know that the
        # invariant part is only threatened by the add effect if the first two
        # invariant parameters are equal and the third parameter is a.

        # The add effect must be balanced in all threatening action
        # applications. We thus must adapt the constraint system such that it
        # prevents restricting solution that set action parameters or
        # quantified variables of the add effect equal to each other or to
        # a specific constant if this is not already implied by the threat.
        params = [p.name for p in itertools.chain(action.parameters,
                                                  add_effect.parameters)]
        param_system = constraints.ConstraintSystem()
        representative = add_cover.get_representative()
        # Dictionary representative maps every term to its representative in
        # the finest equivalence relation induced by the equalities in
        # add_cover. If the equivalence class contains an object, the
        # representative is an object.
        for param in params:
            r = representative.get(param, param)
            if isinstance(r, int) or r[0] == "?":
                # for the add effect being a threat to the invariant, param
                # does not need to be a specific constant. So we may not bind
                # it to a constant when balancing the add effect. We store this
                # information here.
                param_system.add_not_constant(param)
        for (n1, n2) in itertools.combinations(params, 2):
            if representative.get(n1, n1) != representative.get(n2, n2):
                # n1 and n2 don't have to be equivalent to cover the add
                # effect, so we require for the solutions that they do not
                # make n1 and n2 equvalent.
                ineq_disj = constraints.InequalityDisjunction([(n1, n2)])
                param_system.add_inequality_disjunction(ineq_disj)

        for del_effect in del_effects:
            if self._balances(del_effect, add_effect,
                              add_effect_produced_by_pred, add_cover,
                              param_system):
                return False

        # The balance check failed => Generate new candidates.
        self._refine_candidate(add_effect, action, enqueue_func)
        return True

    def _refine_candidate(self, add_effect, action, enqueue_func):
        """Refines the candidate for an add effect that is unbalanced in the
           action and adds the refined one to the queue."""
        part = self.predicate_to_part[add_effect.literal.predicate]
        for del_eff in [eff for eff in action.effects if eff.literal.negated]:
            if del_eff.literal.predicate not in self.predicate_to_part:
                for match in part.possible_matches(add_effect.literal,
                                                   del_eff.literal):
                    enqueue_func(Invariant(self.parts.union((match,))))

    def _balances(self, del_effect, add_effect, produced_by_pred,
                 add_cover, param_system):
        """Returns whether the del_effect is guaranteed to balance the add effect
           where the input is such that:
           - produced_by_pred must be true for the add_effect to be produced,
           - add_cover is an equality conjunction that sets each invariant
             parameter equal to its value in add_effect. These equivalences
             must be true for the add effect threatening the invariant.
           - param_system contains contraints that action and add_effect
             parameters are not fixed to be equivalent or a certain constant
             (except the add effect is otherwise not threat)."""

        balance_system = self._balance_system(add_effect, del_effect,
                                              produced_by_pred)
        if not balance_system:
            # it is impossible to guarantee that every production by add_effect
            # implies a consumption by del effect.
            return False

        # We will overall build a system that is solvable if the delete effect
        # is guaranteed to balance the add effect for this invariant.
        system = constraints.ConstraintSystem()
        system.add_equality_conjunction(add_cover)
        # In every solution, the invariant parameters must equal the
        # corresponding arguments of the add effect atom.

        ensure_cover(system, del_effect.literal, self)
        # In every solution, the invariant parameters must equal the
        # corresponding arguments of the delete effect atom.

        system.extend(balance_system)
        # In every solution a production by the add effect guarantees
        # a consumption by the delete effect.

        system.extend(param_system)
        # A solution may not restrict action parameters (must be balanced
        # independent of the concrete action instantiation).

        if not system.is_solvable():
            return False
        return True

    def _balance_system(self, add_effect, del_effect, literals_by_pred):
        """Returns a constraint system that is solvable if
           - the conjunction of literals occurring as values in dictionary
             literals_by_pred (characterizing a threat for the invariant
             through an actual production by add_effect) implies the
             consumption of the atom of the delete effect, and
           - the produced and consumed atom are different (otherwise by
             add-after-delete semantics, the delete effect would not balance
             the add effect).

           We return None if we detect that the constraint system would never
           be solvable (by an incomplete cheap test).
           """
        system = constraints.ConstraintSystem()
        for literal in itertools.chain(get_literals(del_effect.condition),
                                       [del_effect.literal.negate()]):
            possibilities = []
            # possible equality conjunctions that establish that the literals
            # in literals_by_pred logically imply the current literal.
            for match in literals_by_pred[literal.predicate]:
                if match.negated == literal.negated:
                    # match implies literal iff they agree on each argument
                    ec = constraints.EqualityConjunction(list(zip(literal.args,
                                                                  match.args)))
                    possibilities.append(ec)
            if not possibilities:
                return None
            system.add_equality_DNF(possibilities)

        # if the add effect and the delete effect affect the same predicate
        # then their arguments must differ in at least one position (because of
        # the add-after-delete semantics).
        ensure_inequality(system, add_effect.literal, del_effect.literal)
        return system
