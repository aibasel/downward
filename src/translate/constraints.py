import itertools
from typing import Iterable, List, Tuple

class InequalityDisjunction:
    # disjunction of inequalities
    def __init__(self, parts: List[Tuple[str, str]]):
        self.parts = parts
        assert len(parts)

    def __str__(self):
        disj = " or ".join([f"({v1} != {v2})" for (v1, v2) in self.parts])
        return f"({disj})"


class EqualityConjunction:
    def __init__(self, equalities):
        self.equalities = tuple(equalities)
        # represents a conjunction of expressions ?x = ?y or ?x = d
        # with ?x, ?y being variables and d being a domain value

        self.consistent = None
        self.mapping = None
        self.eq_classes = None

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
        self.eq_classes = eq_classes

    def _compute_mapping(self):
        if not self.eq_classes:
            self._compute_equivalence_classes()

        # create mapping: each key is mapped to the smallest
        # element in its equivalence class (with objects being
        # smaller than variables)
        mapping = {}
        for eq_class in self.eq_classes.values():
            variables = [item for item in eq_class if item.startswith("?")]
            constants = [item for item in eq_class if not item.startswith("?")]
            if len(constants) >= 2:
                self.consistent = False
                self.mapping = None
                return
            if constants:
                set_val = constants[0]
            else:
                set_val = min(variables)
            for entry in eq_class:
                mapping[entry] = set_val
        self.consistent = True
        self.mapping = mapping

    def is_consistent(self):
        if self.consistent is None:
            self._compute_mapping()
        return self.consistent

    def get_mapping(self):
        if self.consistent is None:
            self._compute_mapping()
        return self.mapping


class ConstraintSystem:
    """A ConstraintSystem stores two parts, both talking about the equality or
       inequality of strings (representing objects, variables or invariant
       parameters):
        - equality_DNFs is a list containing lists of EqualityConjunctions.
          Each EqualityConjunction represents an expression of the form
          (x1 = y1 and ... and xn = yn). A list of EqualityConjunctions can be
          interpreted as a disjunction of such expressions.
        - ineq_disjunctions is a list of InequalityDisjunctions. Each of them
          represents a expression of the form (u1 != v1 or ... or um !=i vm).

        We say that the system is solvable if we can pick from each list of
        EqualityConjunctions in equality_DNFs one EquivalenceConjunction such
        that the finest equivalence relation induced by all the equivalences in
        the conjunctions is
        - consistent, i.e. no equivalence class contains more than one object,
          and
        - for every disjunction in ineq_disjunctions there is at least one
          inequality such that the two terms are in different equivalence
          classes."""

    def __init__(self):
        self.equality_DNFs = []
        self.ineq_disjunctions = []

    def __str__(self):
        equality_DNFs = []
        for eq_DNF in self.equality_DNFs:
            disj = " or ".join([str(eq_conjunction)
                                for eq_conjunction in eq_DNF])
            disj = "(%s)" % disj
            equality_DNFs.append(disj)
        equality_part = " and\n".join(equality_DNFs)

        ineq_disjunctions = [str(clause) for clause in self.ineq_disjunctions]
        inequality_part = " and ".join(ineq_disjunctions)
        return f"{equality_part} ({inequality_part})"

    def _combine_equality_conjunctions(self, eq_conjunctions:
                                       Iterable[EqualityConjunction]) -> None:
        all_eq = itertools.chain.from_iterable(c.equalities
                                               for c in eq_conjunctions)
        return EqualityConjunction(all_eq)

    def add_equality_conjunction(self, eq_conjunction: EqualityConjunction):
        self.add_equality_DNF([eq_conjunction])

    def add_equality_DNF(self, equality_DNF: List[EqualityConjunction]) -> None:
        self.equality_DNFs.append(equality_DNF)

    def add_inequality_disjunction(self, ineq_disj: InequalityDisjunction):
        self.ineq_disjunctions.append(ineq_disj)

    def extend(self, other: "ConstraintSystem") -> None:
        self.equality_DNFs.extend(other.equality_DNFs)
        self.ineq_disjunctions.extend(other.ineq_disjunctions)

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
            # combined equality conjunction there is in each inequality
            # disjunction an inequality where the two terms are in different
            # equivalence classes.
            representative = combined.get_mapping()
            for ineq_disjunction in self.ineq_disjunctions:
                if not inequality_disjunction_ok(ineq_disjunction,
                                                 representative):
                    break
            else:
                return True
        return False
