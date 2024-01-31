import itertools
from typing import Iterable, List, Tuple

class InequalityDisjunction:
    def __init__(self, parts: List[Tuple[str, str]]):
        self.parts = parts
        assert len(parts)

    def __str__(self):
        disj = " or ".join([f"({v1} != {v2})" for (v1, v2) in self.parts])
        return f"({disj})"


class EqualityConjunction:
    def __init__(self, equalities: List[Tuple[str, str]]):
        self.equalities = equalities
        # A conjunction of expressions x = y, where x,y are either strings
        # that denote objects or variables, or ints that denote invariant
        # parameters.

        self._consistent = None
        self._representative = None # dictionary
        self._eq_classes = None

    def __str__(self):
        conj = " and ".join([f"({v1} = {v2})" for (v1, v2) in self.equalities])
        return f"({conj})"

    def _compute_equivalence_classes(self):
        eq_classes = {}
        for (v1, v2) in self.equalities:
            c1 = eq_classes.setdefault(v1, {v1})
            c2 = eq_classes.setdefault(v2, {v2})
            if c1 is not c2:
                if len(c2) > len(c1):
                    v1, c1, v2, c2 = v2, c2, v1, c1
                c1.update(c2)
                for elem in c2:
                    eq_classes[elem] = c1
        self._eq_classes = eq_classes

    def _compute_representatives(self):
        if not self._eq_classes:
            self._compute_equivalence_classes()

        # Choose a representative for each equivalence class. Objects are
        # prioritized over variables and ints, but at most one object per
        # equivalence class is allowed (otherwise the conjunction is
        # inconsistent).
        representative = {}
        for eq_class in self._eq_classes.values():
            if next(iter(eq_class)) in representative:
                continue # we already processed this equivalence class
            variables = [item for item in eq_class if isinstance(item, int) or
                         item.startswith("?")]
            objects = [item for item in eq_class if not isinstance(item, int)
                       and not item.startswith("?")]

            if len(objects) >= 2:
                self._consistent = False
                self._representative = None
                return
            if objects:
                set_val = objects[0]
            else:
                set_val = variables[0]
            for entry in eq_class:
                representative[entry] = set_val
        self._consistent = True
        self._representative = representative

    def is_consistent(self):
        if self._consistent is None:
            self._compute_representatives()
        return self._consistent

    def get_representative(self):
        if self._consistent is None:
            self._compute_representatives()
        return self._representative


class ConstraintSystem:
    """A ConstraintSystem stores two parts, both talking about the equality or
       inequality of strings and ints (strings representing objects or
       variables, ints representing invariant parameters):
        - equality_DNFs is a list containing lists of EqualityConjunctions.
          Each EqualityConjunction represents an expression of the form
          (x1 = y1 and ... and xn = yn). A list of EqualityConjunctions can be
          interpreted as a disjunction of such expressions. So
          self.equality_DNFs represents a formula of the form "⋀ ⋁ ⋀ (x = y)"
          as a list of lists of EqualityConjunctions.
        - ineq_disjunctions is a list of InequalityDisjunctions. Each of them
          represents a expression of the form (u1 != v1 or ... or um !=i vm).
        - not_constant is a list of strings.

        We say that the system is solvable if we can pick from each list of
        EqualityConjunctions in equality_DNFs one EquivalenceConjunction such
        that the finest equivalence relation induced by all the equivalences in
        the conjunctions is
        - consistent, i.e. no equivalence class contains more than one object,
        - for every disjunction in ineq_disjunctions there is at least one
          inequality such that the two terms are in different equivalence
          classes.
        - every element of not_constant is not in the same equivalence class
          as a constant.
        We refer to the equivalence relation as the solution of the system."""

    def __init__(self):
        self.equality_DNFs = []
        self.ineq_disjunctions = []
        self.not_constant = []

    def __str__(self):
        equality_DNFs = []
        for eq_DNF in self.equality_DNFs:
            disj = " or ".join([str(eq_conjunction)
                                for eq_conjunction in eq_DNF])
            disj = "(%s)" % disj
            equality_DNFs.append(disj)
        eq_part = " and\n".join(equality_DNFs)

        ineq_disjunctions = [str(clause) for clause in self.ineq_disjunctions]
        ineq_part = " and ".join(ineq_disjunctions)
        return f"{eq_part} ({ineq_part}) (not constant {self.not_constant}"

    def _combine_equality_conjunctions(self, eq_conjunctions:
                                       Iterable[EqualityConjunction]) -> None:
        all_eq = itertools.chain.from_iterable(c.equalities
                                               for c in eq_conjunctions)
        return EqualityConjunction(list(all_eq))

    def add_equality_conjunction(self, eq_conjunction: EqualityConjunction):
        self.add_equality_DNF([eq_conjunction])

    def add_equality_DNF(self, equality_DNF: List[EqualityConjunction]) -> None:
        self.equality_DNFs.append(equality_DNF)

    def add_inequality_disjunction(self, ineq_disj: InequalityDisjunction):
        self.ineq_disjunctions.append(ineq_disj)

    def add_not_constant(self, not_constant: str) -> None:
        self.not_constant.append(not_constant)

    def extend(self, other: "ConstraintSystem") -> None:
        self.equality_DNFs.extend(other.equality_DNFs)
        self.ineq_disjunctions.extend(other.ineq_disjunctions)
        self.not_constant.extend(other.not_constant)

    def is_solvable(self):
        # cf. top of class for explanation
        def inequality_disjunction_ok(ineq_disj, representative):
            for inequality in ineq_disj.parts:
                a, b = inequality
                if representative.get(a, a) != representative.get(b, b):
                    return True
            return False

        for eq_conjunction in itertools.product(*self.equality_DNFs):
            combined = self._combine_equality_conjunctions(eq_conjunction)
            if not combined.is_consistent():
                continue
            # check whether with the finest equivalence relation induced by the
            # combined equality conjunction there is no element of not_constant
            # in the same equivalence class as a constant and that in each
            # inequality disjunction there is an inequality where the two terms
            # are in different equivalence classes.
            representative = combined.get_representative()
            if any(not isinstance(representative.get(s, s), int) and
                   representative.get(s, s)[0] != "?"
                   for s in self.not_constant):
                continue
            if any(not inequality_disjunction_ok(d, representative)
                   for d in self.ineq_disjunctions):
                continue
            return True
        return False
