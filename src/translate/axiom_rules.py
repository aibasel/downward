import options
import pddl
import sccs
import timers

from collections import defaultdict
from itertools import chain


DEBUG = False

class AxiomDependencies(object):
    def __init__(self, axioms):
        if DEBUG:
            assert all(isinstance(axiom.effect, pddl.Atom) for axiom in axioms)
        self.derived_variables = {axiom.effect for axiom in axioms}
        self.positive_dependencies = defaultdict(set)
        self.negative_dependencies = defaultdict(set)
        for axiom in axioms:
            head = axiom.effect
            for body_literal in axiom.condition:
                body_atom = body_literal.positive()
                if body_atom in self.derived_variables:
                    if body_literal.negated:
                        self.negative_dependencies[head].add(body_atom)
                    else:
                        self.positive_dependencies[head].add(body_atom)

    # Remove all information for variables that are not necessary.
    # We do not need to remove single entries from the dicts because if the key
    # (= head of an axiom) is relevant, then all its values (= body of axiom)
    # must be relevant by definition.
    def remove_unnecessary_variables(self, necessary_atoms):
        for var in self.derived_variables:
            if var not in necessary_atoms:
                self.positive_dependencies.pop(var, None)
                self.negative_dependencies.pop(var, None)
        self.derived_variables &= necessary_atoms


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
        self.layer = 0


def handle_axioms(operators, axioms, goals, layer_strategy):
    clusters = compute_clusters(axioms, goals, operators)
    axiom_layers = compute_axiom_layers(clusters, layer_strategy)
    axioms = get_axioms(clusters)
    if DEBUG:
        verify_layering_condition(axioms, axiom_layers)
    return axioms, axiom_layers


def compute_necessary_atoms(dependencies, goals, operators):
    necessary_atoms = set()

    for g in goals:
        g = g.positive()
        if g in dependencies.derived_variables:
            necessary_atoms.add(g)

    for op in operators:
        derived_preconditions = (l.positive() for l in op.precondition if l.positive()
                                 in dependencies.derived_variables)
        necessary_atoms.update(derived_preconditions)

        for condition, effect in chain(op.add_effects, op.del_effects):
            for c in condition:
                c_atom = c.positive()
                if c_atom in dependencies.derived_variables:
                    necessary_atoms.add(c_atom)

    atoms_to_process = list(necessary_atoms)
    while atoms_to_process:
        atom = atoms_to_process.pop()
        for body_atom in chain(dependencies.positive_dependencies[atom],
                               dependencies.negative_dependencies[atom]):
            if body_atom not in necessary_atoms:
                atoms_to_process.append(body_atom)
                necessary_atoms.add(body_atom)
    return necessary_atoms


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

# Expects a list of axioms *with the same head* and returns a subset consisting
# of all non-dominated axioms whose conditions have been cleaned up
# (duplicate elimination).
def compute_simplified_axioms(axioms):
    """Remove duplicate axioms, duplicates within axioms, and dominated axioms."""

    if DEBUG:
        assert len(set(axiom.effect for axiom in axioms)) == 1

    # Remove duplicates from axiom conditions.
    for axiom in axioms:
        axiom.condition = sorted(set(axiom.condition))

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
        dominated_axioms = axioms_by_literal[next(literals)].copy()
        for literal in literals:
            dominated_axioms &= axioms_by_literal[literal]
        for dominated_axiom in dominated_axioms:
            if dominated_axiom != id(axiom):
                axioms_to_skip.add(dominated_axiom)
    return [axiom for axiom in axioms if id(axiom) not in axioms_to_skip]


def compute_clusters(axioms, goals, operators):
    dependencies = AxiomDependencies(axioms)

    # Compute necessary literals and prune unnecessary vars from dependencies.
    necessary_atoms = compute_necessary_atoms(dependencies, goals, operators)
    dependencies.remove_unnecessary_variables(necessary_atoms)

    groups = get_strongly_connected_components(dependencies)
    clusters = [AxiomCluster(group) for group in groups]

    # Compute mapping from variables to their clusters.
    variable_to_cluster = {}
    for cluster in clusters:
        for variable in cluster.variables:
            variable_to_cluster[variable] = cluster

    # Assign axioms to their clusters.
    for axiom in axioms:
        # axiom.effect is derived but might have been pruned
        if axiom.effect in dependencies.derived_variables:
            variable_to_cluster[axiom.effect].axioms[axiom.effect].append(axiom)

    removed = 0
    with timers.timing("Simplifying axioms"):
        for cluster in clusters:
            for variable in cluster.variables:
                old_size = len(cluster.axioms[variable])
                cluster.axioms[variable] = compute_simplified_axioms(cluster.axioms[variable])
                removed += old_size - len(cluster.axioms[variable])
    print("Translator axioms removed by simplifying: %d" % removed)

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


# Clusters must be ordered topologically based on AxiomDependencies.
# Since we need to visit clusters containing variables that occur in the body
# of an atom before we visit the cluster containing the head, we need to
# traverse the clusters in reverse order.
def compute_axiom_layers(clusters, strategy):
    if strategy == "max":
        layer = 0
        for cluster in reversed(clusters):
            cluster.layer = layer
            layer += 1
    elif strategy == "min":
        for cluster in reversed(clusters):
            cluster.layer = compute_single_cluster_layer(cluster)

    layers = dict()
    for cluster in clusters:
        for variable in cluster.variables:
            layers[variable] = cluster.layer
    return layers


def get_axioms(clusters):
    axioms = []
    for cluster in clusters:
        for v in cluster.variables:
            axioms += cluster.axioms[v]
    return axioms


def verify_layering_condition(axioms, axiom_layers):
    # This function is only used for debugging.
    variables_in_heads = set()
    variables_with_layers = set()

    for axiom in axioms:
        head = axiom.effect
        assert not head.negated
        variables_in_heads.add(head)
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

    # 2. For every rule head <- ... cond ... where cond is a literal of
    #    a derived variable, the layer of cond is strictly smaller than the
    #    layer of cond or it is equal and cond is an atom (not a negative
    #    literal).
    print("Verifying 2...")
    for axiom in axioms:
        head = axiom.effect
        head_layer = axiom_layers[head]
        body = axiom.condition
        for cond in body:
            cond_positive = cond.positive()
            if cond_positive in variables_in_heads: # cond is derived variable
                cond_layer = axiom_layers[cond_positive]
                assert (cond_layer <= head_layer), (cond_layer, head_layer)
                # We need the assertion to be on a single line for
                # our error handler to be able to print the line.
                assert (not cond.negated or cond_layer < head_layer), (cond_layer, head_layer)
