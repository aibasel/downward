# -*- coding: utf-8 -*-

"""This module contains a function for simplifying tasks in
finite-domain representation (SASTask). Usage:

    simplify.filter_unreachable_propositions(sas_task)

simplifies `sas_task` in-place. If simplification detects that the
task is unsolvable, the function raises `simplify.Impossible`.

The simplification procedure generates DTGs for the task and then
removes facts that are unreachable from the initial state in a DTG.
Note that such unreachable facts can exist even though we perform a
relaxed reachability analysis before grounding (and DTG reachability
is weaker than relaxed reachability) because the previous relaxed
reachability does not take into account any mutex information, while
PDDL-to-SAS conversion gets rid of certain operators that cannot be
applicable given the mutex information.

Despite the name, the method touches more than the set of facts. For
example, operators that have preconditions on pruned facts are
removed, too. (See also the docstring of
filter_unreachable_propositions.)
"""


from __future__ import print_function

from collections import defaultdict
from itertools import count

DEBUG = False

# TODO:
# This is all quite hackish and would be easier if the translator were
# restructured so that more information is immediately available for
# the propositions, and if propositions had more structure. Directly
# working with int pairs is awkward.


class DomainTransitionGraph(object):
    """Domain transition graphs.

    Attributes:
    - init (int): the initial state value of the DTG variable
    - size (int): the number of values in the domain
    - arcs (defaultdict: int -> set(int)): the DTG arcs (unlabeled)

    There are no transition labels or goal values.

    The intention is that nodes are represented as ints in {1, ...,
    domain_size}, but this is not enforced.

    For derived variables, the "fallback value" that is produced by
    negation by failure should be used for `init`, so that it is
    always considered reachable.
    """

    def __init__(self, init, size):
        """Create a DTG with no arcs."""
        self.init = init
        self.size = size
        self.arcs = defaultdict(set)

    def add_arc(self, u, v):
        """Add an arc from u to v."""
        self.arcs[u].add(v)

    def reachable(self):
        """Return the values reachable from the initial value.
        Represented as a set(int)."""
        queue = [self.init]
        reachable = set(queue)
        while queue:
            node = queue.pop()
            new_neighbors = self.arcs.get(node, set()) - reachable
            reachable |= new_neighbors
            queue.extend(new_neighbors)
        return reachable

    def dump(self):
        """Dump the DTG."""
        print("DTG size:", self.size)
        print("DTG init value:", self.init)
        print("DTG arcs:")
        for source, destinations in sorted(self.arcs.items()):
            for destination in sorted(destinations):
                print("  %d => %d" % (source, destination))


def build_dtgs(task):
    """Build DTGs for all variables of the SASTask `task`.
    Return a list(DomainTransitionGraph), one for each variable.

    For derived variables, we do not consider the axiom bodies, i.e.,
    we treat each axiom as if it were an operator with no
    preconditions. In the case where the only derived variables used
    are binary and all rules change the value from the default value
    to the non-default value, this results in the correct DTG.
    Otherwise, at worst it results in an overapproximation, which
    would not threaten correctness."""

    init_vals = task.init.values
    sizes = task.variables.ranges
    dtgs = [DomainTransitionGraph(init, size)
            for (init, size) in zip(init_vals, sizes)]

    def add_arc(var_no, pre_spec, post):
        """Add a DTG arc for var_no induced by transition pre_spec -> post.
        pre_spec may be -1, in which case arcs from every value
        other than post are added."""
        if pre_spec == -1:
            pre_values = set(range(sizes[var_no])).difference([post])
        else:
            pre_values = [pre_spec]
        for pre in pre_values:
            dtgs[var_no].add_arc(pre, post)

    def get_combined_preconditions(op):
        """Return a dict(int -> int) with the combined prevail and
        preconditions of op.

        Return None if there are internal contradictions (e.g.
        multiple preconditions with different values)."""
        conditions = {}
        def add_condition(var, val):
            """Try adding condition var->val. Return False if this
            leads to contradictory conditions."""
            current_condition = conditions.get(var)
            if current_condition is None:
                conditions[var] = val
            elif current_condition != val:
                return False
            return True
        for var, val in op.prevail:
            if not add_condition(var, val):
                return None
        for var, pre, post, cond in op.pre_post:
            if pre is not None:
                if not add_condition(var, pre):
                    return None
        return conditions

    def get_effective_pre(var_no, conditions, effect_conditions):
        """Return combined information on the conditions on `var_no`
        from operator conditions and effect conditions.

        - conditions: dict(int -> int) containing the combined
          operator prevail and preconditions (see
          get_combined_preconditions)
        - effect_conditions: list(pair(int, int)) containing the
          effect conditions

        Result:
        - -1   if there is no condition on var_no
        - val  if there is a unique condition var_no=val
        - None if there are contradictory conditions on var_no"""

        result = conditions.get(var_no, -1)
        for cond_var_no, cond_val in cond:
            if cond_var_no == var_no:
                if result == -1:
                    # This is the first condition on var_no.
                    result = cond_val
                elif cond_val != result:
                    # We have contradictory conditions on var_no.
                    return None
        return result

    for op in task.operators:
        conditions = get_combined_preconditions(op)
        for var_no, _, post, cond in op.pre_post:
            effective_pre = get_effective_pre(var_no, conditions, cond)
            if effective_pre is not None:
                add_arc(var_no, effective_pre, post)
    for axiom in task.axioms:
        var_no, val = axiom.effect
        add_arc(var_no, -1, val)

    return dtgs


always_false = object()
always_true = object()

class Impossible(Exception):
    pass

class DoesNothing(Exception):
    pass

class VarValueRenaming(object):
    def __init__(self):
        self.new_var_nos = []   # indexed by old var_no
        self.new_values = []    # indexed by old var_no and old value
        self.new_sizes = []     # indexed by new var_no
        self.new_var_count = 0
        self.num_removed_values = 0

    def dump(self):
        old_var_count = len(self.new_var_nos)
        print("variable count: %d => %d" % (
            old_var_count, self.new_var_count))
        print("number of removed values: %d" % self.num_removed_values)
        print("variable conversions:")
        for old_var_no, (new_var_no, new_values) in enumerate(
                zip(self.new_var_nos, self.new_values)):
            old_size = len(new_values)
            new_size = self.new_sizes[new_var_no]
            print("variable %d [size %d] => %d [size %d]" % (
                old_var_no, old_size, new_var_no, new_size))
            for old_value, new_value in enumerate(new_values):
                if new_value is always_false:
                    new_value = "always false"
                elif new_value is always_true:
                    new_value = "always true"
                print("    value %d => %s" % (old_value, new_value))

    def register_variable(self, old_domain_size, init_value, new_domain):
        assert 1 <= len(new_domain) <= old_domain_size
        assert init_value in new_domain
        if len(new_domain) == 1:
            # Remove this variable completely.
            new_values_for_var = [always_false] * old_domain_size
            new_values_for_var[init_value] = always_true
            self.new_var_nos.append(None)
            self.new_values.append(new_values_for_var)
            self.num_removed_values += old_domain_size
        else:
            new_value_counter = count()
            new_values_for_var = []
            for value in range(old_domain_size):
                if value in new_domain:
                    new_values_for_var.append(next(new_value_counter))
                else:
                    self.num_removed_values += 1
                    new_values_for_var.append(always_false)
            new_size = next(new_value_counter)
            assert new_size == len(new_domain)

            self.new_var_nos.append(self.new_var_count)
            self.new_values.append(new_values_for_var)
            self.new_sizes.append(new_size)
            self.new_var_count += 1

    def apply_to_task(self, task):
        if DEBUG:
            self.dump()
        self.apply_to_variables(task.variables)
        self.apply_to_mutexes(task.mutexes)
        self.apply_to_init(task.init)
        self.apply_to_goals(task.goal.pairs)
        self.apply_to_operators(task.operators)
        self.apply_to_axioms(task.axioms)

    def apply_to_variables(self, variables):
        variables.ranges = self.new_sizes
        new_axiom_layers = [None] * self.new_var_count
        for old_no, new_no in enumerate(self.new_var_nos):
            if new_no is not None:
                new_axiom_layers[new_no] = variables.axiom_layers[old_no]
        assert None not in new_axiom_layers
        variables.axiom_layers = new_axiom_layers
        self.apply_to_value_names(variables.value_names)

    def apply_to_value_names(self, value_names):
        new_value_names = [[None] * size for size in self.new_sizes]
        for var_no, values in enumerate(value_names):
            for value, value_name in enumerate(values):
                new_var_no, new_value = self.translate_pair((var_no, value))
                if new_value is always_true:
                    if DEBUG:
                        print("Removed true proposition: %s" % value_name)
                elif new_value is always_false:
                    if DEBUG:
                        print("Removed false proposition: %s" % value_name)
                else:
                    new_value_names[new_var_no][new_value] = value_name
        assert all((None not in value_names) for value_names in new_value_names)
        value_names[:] = new_value_names

    def apply_to_mutexes(self, mutexes):
        new_mutexes = []
        for mutex in mutexes:
            new_facts = []
            for var, val in mutex.facts:
                new_var_no, new_value = self.translate_pair((var, val))
                if (new_value is not always_true and
                    new_value is not always_false):
                    new_facts.append((new_var_no, new_value))
            if len(new_facts) >= 2:
                mutex.facts = new_facts
                new_mutexes.append(mutex)
        mutexes[:] = new_mutexes

    def apply_to_init(self, init):
        init_pairs = list(enumerate(init.values))
        try:
            self.translate_pairs_in_place(init_pairs)
        except Impossible:
            assert False, "Initial state impossible? Inconceivable!"
        new_values = [None] * self.new_var_count
        for new_var_no, new_value in init_pairs:
            new_values[new_var_no] = new_value
        assert None not in new_values
        init.values = new_values

    def apply_to_goals(self, goals):
        # This may propagate Impossible up.
        self.translate_pairs_in_place(goals)

    def apply_to_operators(self, operators):
        new_operators = []
        num_removed = 0
        for op in operators:
            try:
                self.apply_to_operator(op)
            except (Impossible, DoesNothing):
                num_removed += 1
                if DEBUG:
                    print("Removed operator: %s" % op.name)
            else:
                new_operators.append(op)
        print("%d operators removed" % num_removed)
        operators[:] = new_operators

    def apply_to_axioms(self, axioms):
        new_axioms = []
        num_removed = 0
        for axiom in axioms:
            try:
                self.apply_to_axiom(axiom)
            except (Impossible, DoesNothing):
                num_removed += 1
                if DEBUG:
                    print("Removed axiom:")
                    axiom.dump()
            else:
                new_axioms.append(axiom)
        print("%d axioms removed" % num_removed)
        axioms[:] = new_axioms

    def apply_to_operator(self, op):
        op.dump()

        # The prevail translation may generate an Impossible exception,
        # which is propagated up.
        self.translate_pairs_in_place(op.prevail)
        new_pre_post = []
        for entry in op.pre_post:
            try:
                new_pre_post.append(self.translate_pre_post(entry))
                # This may raise Impossible if "pre" is always false.
                # This is then propagated up.
            except DoesNothing:
                # Conditional effect that is impossible to trigger, or
                # effect that sets an always true value. Swallow this.
                pass
                # TODO: Possible bug here: "DoesNothing" indicates
                # that we have a conditional effect that is impossible
                # to trigger, but I don't think this means we can
                # ignore the whole pre_post entry. If there is a
                # nontrivial precondition associated with the entry,
                # then we must consider it, as the precondition part
                # of the entry has nothing to do with the conditional
                # effect part.
        op.pre_post = new_pre_post
        if not new_pre_post:
            raise DoesNothing

    def apply_to_axiom(self, axiom):
        # The following line may generate an Impossible exception,
        # which is propagated up.
        self.translate_pairs_in_place(axiom.condition)
        new_var, new_value = self.translate_pair(axiom.effect)
        # If the new_value is always false, then the condition must
        # have been impossible.
        assert not new_value is always_false
        if new_value is always_true:
            raise DoesNothing
        axiom.effect = new_var, new_value

    def translate_pre_post(self, pre_post_tuple):
        (var_no, pre, post, cond) = pre_post_tuple
        new_var_no, new_post = self.translate_pair((var_no, post))
        if pre == -1:
            new_pre = -1
        else:
            _, new_pre = self.translate_pair((var_no, pre))
        if new_pre is always_false:
            raise Impossible
        try:
            self.translate_pairs_in_place(cond)
        except Impossible:
            # Explanation for the following assertion: we want to get
            # rid of the postcondition, but not the precondition. That
            # is, the precondition should become a prevail condition.
            # Our calling code currently does not support this (and in
            # any case it's not really clear what should happen if the
            # same precondition occurs as part of multiple conditional
            # effects etc. -- is it still a precondition then?) The
            # correct fix for this is to get rid of the
            # prevail/precondition distinction, which is very fuzzy in
            # the presence of conditional effects.
            assert new_pre == -1 or new_pre is always_true, "we're in trouble"
            raise DoesNothing

        if new_post is always_false:
            # Our analysis shows that the effect is impossible. If our
            # analysis isn't buggy, this must imply that the effect
            # condition, in conjunction with the operator
            # precondition, never triggers. We trust this analysis and
            # ignore the effect. A more defensive programming style
            # would perhaps try to understand more deeply what is
            # going on here.

            # Regarding the assertion, see the other assertion with
            # the same error message above.
            assert new_pre == -1 or new_pre is always_true, "we're in trouble"
            raise DoesNothing
        if new_pre is always_true:
            assert new_post is always_true
            raise DoesNothing
        elif new_post is always_true:
            assert new_pre == -1
            raise DoesNothing
        return new_var_no, new_pre, new_post, cond

    def translate_pair(self, fact_pair):
        (var_no, value) = fact_pair
        new_var_no = self.new_var_nos[var_no]
        new_value = self.new_values[var_no][value]
        return new_var_no, new_value

    def translate_pairs_in_place(self, pairs):
        new_pairs = []
        for pair in pairs:
            new_var_no, new_value = self.translate_pair(pair)
            if new_value is always_false:
                raise Impossible
            elif new_value is not always_true:
                assert new_var_no is not None
                new_pairs.append((new_var_no, new_value))
        pairs[:] = new_pairs

def build_renaming(dtgs):
    renaming = VarValueRenaming()
    for dtg in dtgs:
        renaming.register_variable(dtg.size, dtg.init, dtg.reachable())
    return renaming


def filter_unreachable_propositions(sas_task):
    """We remove unreachable propositions and then prune variables
    with only one value.

    Examples of things that are pruned:
    - Constant propositions that are not detected in instantiate.py
      because instantiate.py only reasons at the predicate level, and some
      predicates such as "at" in Depot are constant for some objects
      (hoists), but not others (trucks).

      Example: "at(hoist1, distributor0)" and the associated variable
      in depots-01.

    - "none of those" values that are unreachable.

      Example: at(truck1, ?x) = <none of those> in depots-01.

    - Certain values that are relaxed reachable but detected as
      unreachable after SAS instantiation because the only operators
      that set them have inconsistent preconditions.

      Example: on(crate0, crate0) in depots-01.
    """

    dtgs = build_dtgs(sas_task)
    renaming = build_renaming(dtgs)
    # apply_to_task may raise Impossible if the goal is detected as
    # unreachable. We let the exception propagate to the caller.
    renaming.apply_to_task(sas_task)
    print("%d propositions removed" % renaming.num_removed_values)
