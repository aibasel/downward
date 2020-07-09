import options
import pddl
import sccs
import timers

from collections import defaultdict, OrderedDict
from itertools import chain


DEBUG = False


def collect_axiom_heads(axioms):
    """Return derived variables that occur as heads in axioms,
    as a set of pddl.Atom."""
    result = set()
    for axiom in axioms:
        # Verify that we only get axioms whose head is a *positive*
        # literal, not a negative one.
        if DEBUG:
            assert isinstance(axiom.effect, pddl.Atom)
        result.add(axiom.effect)
    return result


class AxiomDependencies(object):
    def __init__(self, axioms):
        self.derived_variables = collect_axiom_heads(axioms)
        self.positive_dependencies = {atom: set() for atom in self.derived_variables}
        self.negative_dependencies = {atom: set() for atom in self.derived_variables}
        for axiom in axioms:
            head = axiom.effect
            for body_literal in axiom.condition:
                body_atom = body_literal.positive()
                if body_atom in self.derived_variables:
                    if body_literal.negated:
                        self.negative_dependencies[head].add(body_atom)
                    else:
                        self.positive_dependencies[head].add(body_atom)

    # Remove all information for variables whose literals are not necessary.
    # If the head is relevant, then all literals in the body are relevant.
    # Thus it is not necessary to filter the entries in the dicts.
    # TODO: improve comment
    def remove_unnecessary_variables(self, necessary_literals):
        vars_to_remove = [v for v in self.derived_variables if (v not in necessary_literals and v.negate() not in necessary_literals)]
        for var in vars_to_remove:
           self.derived_variables.remove(var)
           del self.positive_dependencies[var]
           del self.negative_dependencies[var]


class AxiomCluster(object):
    def __init__(self, derived_variables):
        self.variables = derived_variables
        self.axioms = dict((v, []) for v in derived_variables)
        # Positive children will be populated with clusters that contain an
        # atom that occurs in the body of an axiom whose head is from this
        # cluster. Negative children analogous for atoms that occur negated
        # in the body.
        self.positive_children = set()
        self.negative_children = set()
        self.needed_negative = False
        self.layer = 0


def handle_axioms(operators, axioms, goals, layer_strategy):
    clusters = compute_clusters(axioms, goals, operators)
    axiom_layers = compute_axiom_layers(clusters, layer_strategy)

    with timers.timing("Computing negative axioms"):
        compute_negative_axioms(clusters)

    axioms = get_axioms(clusters)
    if DEBUG:
        verify_layering_condition(axioms, axiom_layers)


    return axioms, axiom_layers


def compute_necessary_literals(dependencies, goals, operators):
    necessary_literals = set()

    for g in goals:
        if g.positive() in dependencies.derived_variables:
            necessary_literals.add(g)

    for op in operators:
        derived_pres = (l for l in op.precondition if l.positive() in dependencies.derived_variables)
        necessary_literals.update(derived_pres)

        # TODO: why do we add both the literal and its negation here?
        for condition, effect in chain(op.add_effects, op.del_effects):
            for c in condition:
                if c.positive() in dependencies.derived_variables:
                    necessary_literals.add(c)
                    necessary_literals.add(c.negate())

    literals_to_process = list(necessary_literals)
    while literals_to_process:
        l = literals_to_process.pop()
        atom = l.positive()
        for body_atom in dependencies.positive_dependencies[atom]:
            l2 = body_atom.negate() if l.negated else body_atom
            if l2 not in necessary_literals:
                literals_to_process.append(l2)
                necessary_literals.add(l2)
        for body_atom in dependencies.negative_dependencies[atom]:
            l2 = body_atom if l.negated else body_atom.negate()
            if l2 not in necessary_literals:
                literals_to_process.append(l2)
                necessary_literals.add(l2)

    return necessary_literals


# Compute strongly connected components of the dependency graph.
# In order to receive a deterministic result, we first sort the variables.
# We then build adjacency lists over the variable indices based on dependencies.
def get_strongly_connected_components(dependencies):
    sorted_vars = sorted(dependencies.derived_variables)
    variable_to_index = {var: index for index, var in enumerate(sorted_vars)}

    adjacency_list = []
    for derived_var in sorted_vars:
      pos = dependencies.positive_dependencies[derived_var]
      neg = dependencies.negative_dependencies[derived_var]
      indices = [variable_to_index[atom] for atom in sorted(pos.union(neg))]
      adjacency_list.append(indices)

    index_groups = sccs.get_sccs_adjacency_list(adjacency_list)
    groups = [[sorted_vars[i] for i in g] for g in index_groups]
    return groups


def remove_duplicates(alist):
    next_elem = 1
    for i in range(1, len(alist)):
        if alist[i] != alist[i - 1]:
            alist[next_elem] = alist[i]
            next_elem += 1
    alist[next_elem:] = []


def simplify(axioms):
    """Remove duplicate axioms, duplicates within axioms, and dominated axioms."""

    # Remove duplicates from axiom conditions.
    for axiom in axioms:
        axiom.condition.sort()
        remove_duplicates(axiom.condition)

    # Remove dominated axioms.
    axioms_to_skip = set()
    axioms_by_literal = defaultdict(set)
    for axiom in axioms:
        if axiom.effect in axiom.condition:
            axioms_to_skip.add(id(axiom))
        else:
            for literal in axiom.condition:
                axioms_by_literal[literal].add(id(axiom))

    for axiom in axioms:
        if id(axiom) in axioms_to_skip:
            continue   # Required to keep one of multiple identical axioms.
        if not axiom.condition:  # empty condition: dominates everything
            return [axiom]
        literals = iter(axiom.condition)
        dominated_axioms = axioms_by_literal[next(literals)]
        for literal in literals:
            dominated_axioms &= axioms_by_literal[literal]
        for dominated_axiom in dominated_axioms:
            if dominated_axiom != id(axiom):
                axioms_to_skip.add(dominated_axiom)
    return [axiom for axiom in axioms if id(axiom) not in axioms_to_skip]


def compute_clusters(axioms, goals, operators):
    dependencies = AxiomDependencies(axioms)

    # Compute necessary literals and prune unnecessary vars from dependencies.
    necessary_literals = compute_necessary_literals(dependencies, goals, operators)
    dependencies.remove_unnecessary_variables(necessary_literals)

    groups = get_strongly_connected_components(dependencies)
    clusters = [AxiomCluster(group) for group in groups]
    # TODO: why do we need to reverse here?
    clusters.reverse()

    # Compute mapping from variables to their clusters and set needed_negative.
    variable_to_cluster = {}
    for cluster in clusters:
        for variable in cluster.variables:
            variable_to_cluster[variable] = cluster
            if variable.negate() in necessary_literals:
                cluster.needed_negative = True

    # Assign axioms to their clusters.
    for axiom in axioms:
        # axiom.effect is derived but might have been pruned
        if axiom.effect in dependencies.derived_variables:
            variable_to_cluster[axiom.effect].axioms[axiom.effect].append(axiom)

    with timers.timing("Simplifying axioms"):
      for cluster in clusters:
          for variable in cluster.variables:
              simplify(cluster.axioms[variable])

    # Create links between clusters (positive dependencies).
    for from_variable, depends_on in dependencies.positive_dependencies.items():
        from_cluster = variable_to_cluster[from_variable]
        for to_variable in depends_on:
            to_cluster = variable_to_cluster[to_variable]
            if from_cluster is not to_cluster:
                from_cluster.positive_children.add(to_cluster)

    # Create links between clusters (negative dependencies).
    for from_variable, depends_on in dependencies.negative_dependencies.items():
        from_cluster = variable_to_cluster[from_variable]
        for to_variable in depends_on:
            to_cluster = variable_to_cluster[to_variable]
            if from_cluster is to_cluster:
                raise ValueError("axioms are not stratifiable")
            from_cluster.negative_children.add(to_cluster)

    return clusters


# Assign every cluster the smallest possible layer.
def compute_single_cluster_layer(cluster):
    layer = 0
    for pos_child in cluster.positive_children:
        layer = max(pos_child.layer, layer)
    for neg_child in cluster.negative_children:
        layer = max(neg_child.layer + 1, layer)
    return layer


# This assumes that the clusters are in topological order.
def compute_axiom_layers(clusters, strategy):
    if strategy == "max":
        layer = 0
        for cluster in clusters:
            cluster.layer = layer
            layer += 1
    elif strategy == "min":
        for cluster in clusters:
            cluster.layer = compute_single_cluster_layer(cluster)

    layers = dict()
    for cluster in clusters:
        for variable in cluster.variables:
            layers[variable] = cluster.layer
    return layers


def compute_negative_axioms(clusters):
    for cluster in clusters:
        if cluster.needed_negative:
            if len(cluster.variables) > 1:
                # If the cluster contains several variables we need to
                # overapproximate when a derived predicate can become negative.
                # We do this by simply allowing it to always become negative
                # unconditionally.
                # The successor generator in the search component does not
                # consider these overapproximations since they set the derived
                # variable to its default value (false).
                # The reason for adding the overapproximation at all is that
                # some heuristics (e.g. relaxation and causal graph heuristics)
                # use them to derive that the negated value is reachable.
                for variable in cluster.variables:
                    name = cluster.axioms[variable][0].name
                    negated_axiom = pddl.PropositionalAxiom(name, [], variable.negate())
                    cluster.axioms[variable].append(negated_axiom)
            else:
                variable = next(iter(cluster.variables))
                negated_axioms = negate(cluster.axioms[variable])
                cluster.axioms[variable] += negated_axioms


def negate(axioms):
    assert axioms
    result = [pddl.PropositionalAxiom(axioms[0].name, [], axioms[0].effect.negate())]
    for axiom in axioms:
        condition = axiom.condition
        if len(condition) == 0:
            # The derived fact we want to negate is triggered with an
            # empty condition, so it is always true and its negation
            # is always false.
            return []
        elif len(condition) == 1:  # Handle easy special case quickly.
            new_literal = condition[0].negate()
            for result_axiom in result:
                result_axiom.condition.append(new_literal)
        else:
            new_result = []
            for literal in condition:
                literal = literal.negate()
                for result_axiom in result:
                    new_axiom = result_axiom.clone()
                    new_axiom.condition.append(literal)
                    new_result.append(new_axiom)
            result = new_result
    result = simplify(result)
    return result


def get_axioms(clusters):
    axioms = []
    for cluster in clusters:
        for v in cluster.variables:
            axioms += cluster.axioms[v]
    return axioms


def verify_layering_condition(axioms, axiom_layers):
    # This function is only used for debugging.
    variables_in_heads = set()
    literals_in_heads = set()
    variables_with_layers = set()

    for axiom in axioms:
        head = axiom.effect
        variables_in_heads.add(head.positive())
        literals_in_heads.add(head)
    variables_with_layers = set(axiom_layers.keys())

    # 1. A variable has a defined layer iff it appears in a head.
    #    (This is stricter than it needs to be; we could allow
    #    derived variables that are never generated by a rule.
    #    But this test follows the axiom simplification step, and
    #    after simplification this should not be too strict.)
    #    All layers are integers and at least 0.
    #    (Note: the "-1" layer for non-derived variables is
    #    set elsewhere.)
    print("Verifying 1...")
    assert variables_in_heads == variables_with_layers
    for atom, layer in axiom_layers.items():
        assert isinstance(layer, int)
        assert layer >= 0

    # 2. For every rule head <- ... cond ... where cond is a literal
    #    of a derived variable where the layer of head is equal to
    #    the layer of cond, cond occurs with the same polarity in heads.
    #
    # Note regarding issue454 and issue453: Because of the negated axioms
    # mentioned in these issues, a derived variable may appear with *both*
    # polarities in heads. This makes this test less strong than it would
    # be otherwise. When these issues are addressed and axioms only occur
    # with one polarity in heads, this test will remain correct in its
    # current form, but it will be able to detect more violations of the
    # layering property.
    print("Verifying 2...")
    for axiom in axioms:
        head = axiom.effect
        head_positive = head.positive()
        body = axiom.condition
        for cond in body:
            cond_positive = cond.positive()
            if (cond_positive in variables_in_heads and
                axiom_layers[cond_positive] == axiom_layers[head_positive]):
                assert cond in literals_in_heads

    # 3. For every rule head <- ... cond ... where cond is a literal
    #    of a derived variable, the layer of head is greater or equal
    #    to the layer of cond.
    print("Verifying 3...")
    for axiom in axioms:
        head = axiom.effect
        head_positive = head.positive()
        body = axiom.condition
        for cond in body:
            cond_positive = cond.positive()
            if cond_positive in variables_in_heads:
                # We need the assertion to be on a single line for
                # our error handler to be able to print the line.
                assert (axiom_layers[cond_positive] <= axiom_layers[head_positive]), (axiom_layers[cond_positive], axiom_layers[head_positive])
