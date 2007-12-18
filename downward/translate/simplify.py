# -*- coding: utf-8 -*-

from collections import defaultdict
from itertools import count
import sys

DEBUG = False

# TODO:
# This is all quite hackish and would be easier if the translator were
# restructured so that more information is immediately available for
# the propositions, and if propositions had more structure. Directly
# working with int pairs is awkward.
#
# It might also be buggy. In particular, the distinction between
# "trivially false" and "trivially true" conditions below is not
# obvious and I'm not sure if it's based on fact. Similarly for the
# throwing away of "effects on constants" in translate_pre_post.

class DomainTransitionGraph(object):
    def __init__(self, init, size):
        self.init = init
        self.size = size
        self.arcs = defaultdict(set)

    def add_arc(self, u, v):
        self.arcs[u].add(v)

    def reachable(self):
        queue = [self.init]
        reachable = set(queue)
        while queue:
            node = queue.pop()
            new_neighbors = self.arcs.get(node, set()) - reachable
            reachable |= new_neighbors
            queue.extend(new_neighbors)
        return reachable
        
    def dump(self):
        print "SIZE", self.size
        print "INIT", self.init
        print "ARCS:"
        for source, destinations in sorted(self.arcs.items()):
            for destination in sorted(destinations):
                print "  %d => %d" % (source, destination)


def build_dtgs(task):
    init_vals = task.init.values
    sizes = task.variables.ranges
    dtgs = [DomainTransitionGraph(init, size)
            for (init, size) in zip(init_vals, sizes)]

    def add_arc(var_no, pre_spec, post):
        if pre_spec == -1:
            pre_values = set(range(sizes[var_no])).difference([post])
        else:
            pre_values = [pre_spec]
        for pre in pre_values:
            dtgs[var_no].add_arc(pre, post)

    for op in task.operators:
        for var_no, pre_spec, post, cond in op.pre_post:
            add_arc(var_no, pre_spec, post)
    for axiom in task.axioms:
        var_no, val = axiom.effect
        add_arc(var_no, -1, val)

    return dtgs


class VarValueRenaming(object):
    def __init__(self):
        self.new_var_nos = []   # indexed by old var_no
        self.new_values = []    # indexed by old var_no and old value
        self.new_sizes = []     # indexed by new var_no
        self.new_var_count = 0
        self.num_removed_values = 0

    def register_variable(self, old_domain_size, new_domain):
        assert 1 <= len(new_domain) <= old_domain_size
        if len(new_domain) == 1:
            # Remove this variable completely.
            self.new_var_nos.append(None)
            self.new_values.append([None] * old_domain_size)
            self.num_removed_values += old_domain_size
        else:
            new_value_counter = count()
            new_values_for_var = []
            for value in range(old_domain_size):
                if value in new_domain:
                    new_values_for_var.append(new_value_counter.next())
                else:
                    self.num_removed_values += 1
                    new_values_for_var.append(None)
            new_size = new_value_counter.next()
            assert new_size == len(new_domain)

            self.new_var_nos.append(self.new_var_count)
            self.new_values.append(new_values_for_var)
            self.new_sizes.append(new_size)
            self.new_var_count += 1

    def apply_to_task(self, task):
        self.apply_to_variables(task.variables)
        self.apply_to_init(task.init)
        self.apply_to_goals(task.goal.pairs)
        for op in task.operators:
            self.apply_to_operator(op)
        for axiom in task.axioms:
            self.apply_to_axiom(axiom)

    def apply_to_variables(self, variables):
        variables.ranges = self.new_sizes
        new_axiom_layers = [None] * self.new_var_count
        for old_no, new_no in enumerate(self.new_var_nos):
            if new_no is not None:
                new_axiom_layers[new_no] = variables.axiom_layers[old_no]
        assert None not in new_axiom_layers
        variables.axiom_layers = new_axiom_layers

    def apply_to_init(self, init):
        new_values = [None] * self.new_var_count
        for pair in enumerate(init.values):
            try:
                new_var_no, new_value= self.translate_pair(pair)
            except ValueError:
                pass
            else:
                new_values[new_var_no] = new_value
        assert None not in new_values
        init.values = new_values

    def apply_to_goals(self, goals):
        try:
            goals[:] = map(self.translate_pair, goals)
        except ValueError:
            raise SystemExit("trivially unsolvable")

    def apply_to_operator(self, op):
        new_prevail = []
        for pair in op.prevail:
            try:
                new_pair = self.translate_pair(pair)
            except ValueError: 
                # Remove trivially true preconditions.
                pass
            else:
                new_prevail.append(new_pair)
        op.prevail = new_prevail

        new_pre_post = map(self.translate_pre_post, op.pre_post)
        op.pre_post = [pre_post for pre_post in new_pre_post
                       if pre_post is not None]

    def apply_to_axiom(self, axiom):
        axiom.condition = map(self.translate_pair, axiom.condition)
        axiom.effect = self.translate_pair(axiom.effect)

    def translate_pair(self, (var_no, value)):
        new_var_no = self.new_var_nos[var_no]
        new_value = self.new_values[var_no][value]
        if new_var_no is None or new_value is None:
            raise ValueError
        return new_var_no, new_value

    def translate_pre_post(self, (var_no, pre, post, cond)):
        if self.new_var_nos[var_no] is None:
            # Effect on a constant. (Happens in Schedule, e.g.,
            # where initially cylindrical objects will always stay
            # cylindrical, but there are effects that set the value
            # again.
            return None
        new_var_no, new_post = self.translate_pair((var_no, post))
        if pre == -1:
            new_pre = -1
        else:
            new_pre = self.new_values[var_no][pre]
        new_cond = map(self.translate_pair, cond)
        return new_var_no, new_pre, new_post, new_cond    

    def apply_to_translation_key(self, translation_key):
        new_key = [[None] * size for size in self.new_sizes]
        for var_no, value_names in enumerate(translation_key):
            for value, value_name in enumerate(value_names):
                new_var_no = self.new_var_nos[var_no]
                new_value = self.new_values[var_no][value]
                if new_var_no is None or new_value is None:
                    if DEBUG:
                        print "Removed proposition: %s" % value_name
                else:
                    new_key[new_var_no][new_value] = value_name
        assert all((None not in value_names) for value_names in new_key)
        translation_key[:] = new_key

    def apply_to_mutex_key(self, mutex_key):
        new_key = []
        for group in mutex_key:
            new_group = []
            for var, val, name in group:
                new_var_no = self.new_var_nos[var]
                new_value = self.new_values[var][val]
                if new_var_no is None or new_value is None:
                    if DEBUG:
                        print "Removed proposition: %s" % name
                else:
                    new_group.append((new_var_no, new_value, name))
            if len(new_group) > 0:
                new_key.append(new_group)
        mutex_key[:] = new_key
        

def build_renaming(dtgs):
    renaming = VarValueRenaming()
    for dtg in dtgs:
        renaming.register_variable(dtg.size, dtg.reachable())
    return renaming


def dump_translation_key(translation_key):
    for var_no, values in enumerate(translation_key):
        print "var %d:" % var_no
        for value_no, value in enumerate(values):
            print "%2d: %s" % (value_no, value)

def filter_unreachable_propositions(sas_task, mutex_key, translation_key):
    print "Detecting unreachable propositions...",
    sys.stdout.flush()
    if DEBUG:
        print

    # This procedure is a bit of an afterthought, and doesn't fit the
    # overall architecture of the translator too well. We filter away
    # unreachable propositions here, and then prune away variables
    # with only one value left.
    # 
    # Examples of things that are filtered away:
    # - Constant propositions that are not detected in instantiate.py
    #   because it only reasons at the predicate level, and some
    #   predicates such as "at" in Depot is constant for some objects
    #   (hoists), but not others (trucks).
    #   Example: "at(hoist1, distributor0)" and the associated
    #            variable in depots-01.
    # - "none of those" values that are unreachable.
    #   Example: at(truck1, ?x) = <none of those> in depots-01.
    # - Certain values that are relaxed reachable but detected as
    #   unreachable after SAS instantiation because the only operators
    #   that set them have inconsistent preconditions.
    #   Example: on(crate0, crate0) in depots-01.

    # dump_translation_key(translation_key)
    dtgs = build_dtgs(sas_task)
    renaming = build_renaming(dtgs)
    renaming.apply_to_task(sas_task)
    renaming.apply_to_translation_key(translation_key)
    renaming.apply_to_mutex_key(mutex_key)
    print "%d propositions removed." % renaming.num_removed_values
