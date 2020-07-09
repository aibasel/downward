import options
import pddl
import sccs
import timers

from collections import defaultdict, OrderedDict


DEBUG = False


def collect_axiom_heads(axioms):
    """Return derived variables that occur as heads in axioms,
    as a set of pddl.Atom."""
    result = set()
    for axiom in axioms:
        # Verify that we only get axioms whose head is a *positive*
        # literal, not a negative one.
        assert isinstance(axiom.effect, pddl.Atom)
        result.add(axiom.effect)
    return result


class AxiomDependencies(object):
    def __init__(self, axioms):
        # We use an OrderedDict here so that the topological order that
        # we get for the SCCs derived from the dependencies is deterministic.
        self.derived_variables = sorted(collect_axiom_heads(axioms))
        self.all_dependencies = OrderedDict({atom: set() for atom in self.derived_variables})
        self.positive_dependencies = OrderedDict({atom: set() for atom in self.derived_variables})
        self.negative_dependencies = OrderedDict({atom: set() for atom in self.derived_variables})
        for axiom in axioms:
            head = axiom.effect
            for body_literal in axiom.condition:
                body_atom = body_literal.positive()
                if body_atom in self.derived_variables:
                    self.all_dependencies[head].add(body_atom)
                    if body_literal.negated:
                        self.negative_dependencies[head].add(body_atom)
                    else:
                        self.positive_dependencies[head].add(body_atom)


class AxiomCluster(object):
    def __init__(self, variables):
        self.variables = variables
        self.axioms =  dict((v, []) for v in variables)
        self.positive_children = set()
        self.negative_children = set()
        self.needed_positive = False  # will be set by remove_unnecessary_clusters
        self.needed_negative = False  # will be set by remove_unnecessary_clusters
        self.layer = 0  # will be set by compute_axiom_layers
        self.inverted = False  # will be set by compute_negative_axioms if we set the default value of the cluster to True


def compute_clusters(axioms, dependencies):
    groups = sccs.get_sccs_adjacency_dict(dependencies.all_dependencies)
    clusters = [AxiomCluster(group) for group in groups]

    # Compute mapping from variables to their clusters.
    variable_to_cluster = {}
    for cluster in clusters:
        for variable in cluster.variables:
            variable_to_cluster[variable] = cluster

    # Assign axioms to their clusters.
    for axiom in axioms:
        variable_to_cluster[axiom.effect].axioms[axiom.effect].append(axiom)

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

    return clusters, variable_to_cluster


def remove_unnecessary_clusters(clusters, goals, operators, variable_to_cluster):
    depends_on = defaultdict(set)
    necessary_literals = set()

    for g in goals:
        if g.positive() in variable_to_cluster:
            necessary_literals.add(g)
    
    for op in operators:
        derived_pres = [l for l in op.precondition if l.positive() in variable_to_cluster]
        
        for condition, effect in op.add_effects:
            derived_conds = [c for c in condition if c.positive() in variable_to_cluster]
            effect_derived = effect.positive() in variable_to_cluster
            if effect_derived:
                depends_on[effect].update(derived_pres)
                depends_on[effect].update(derived_conds)
            else:
                necessary_literals.update(derived_pres)
                necessary_literals.update(derived_conds)
            for c in derived_conds:
                if effect_derived:
                    depends_on[effect.negate()].add(c.negate())
                else:
                    necessary_literals.add(c.negate())
                
        for condition, effect in op.del_effects:
            derived_conds = [c for c in condition if c.positive() in variable_to_cluster]
            effect_derived = effect.positive() in variable_to_cluster

            if effect_derived:
                depends_on[effect.negate()].update(derived_pres)
                depends_on[effect.negate()].update(derived_conds)
            else:
                necessary_literals.update(derived_pres)
                necessary_literals.update(derived_conds)
            for c in derived_conds:
                if effect_derived:
                    depends_on[effect].add(c.negate())
                else:
                    necessary_literals.add(c.negate())
    
    for cluster in clusters:
        for v in cluster.variables:
            for axiom in cluster.axioms[v]:
                derived_conds = [c for c in axiom.condition if c.positive() in variable_to_cluster]
                depends_on[axiom.effect].update(derived_conds)
                for c in derived_conds:
                    depends_on[axiom.effect.negate()].add(c.negate())
    
    queue = list(necessary_literals)

    while queue:
        l = queue.pop()
        if l.negated:
            variable_to_cluster[l.negate()].needed_negative = True
        else:
            variable_to_cluster[l].needed_positive = True
        for l2 in depends_on[l]:
            if l2 not in necessary_literals:
                queue.append(l2)
                necessary_literals.add(l2)

    return [c for c in clusters if c.needed_positive or c.needed_negative]


def get_only_negative_needed_variables(clusters):
    variables = []
    for cluster in clusters:
        if cluster.needed_negative and not cluster.needed_positive:
            variables += cluster.variables
    return variables


def get_axioms(clusters):
    axioms = []
    for cluster in clusters:
        for v in cluster.variables:
            axioms += cluster.axioms[v]
    return axioms


def handle_axioms(operators, axioms, goals, layer_strategy):
    dependencies = AxiomDependencies(axioms)
    clusters, variable_to_cluster = compute_clusters(axioms, dependencies)
    clusters = remove_unnecessary_clusters(clusters, goals, operators, variable_to_cluster)

    with timers.timing("Simplifying axioms"):
        simplify_axioms(clusters)  # dominance testing and duplicate elimination

    with timers.timing("Computing negative axioms"):
        axiom_init = compute_negative_axioms(clusters)
    
    axiom_layers = compute_axiom_layers(clusters, layer_strategy)
    axioms = get_axioms(clusters)
    if DEBUG:
        verify_layering_condition(axioms, set(axiom_init), axiom_layers)
    return axioms, list(axiom_init), axiom_layers


def verify_layering_condition(axioms, axiom_init, axiom_layers):
    # This function is only used for debugging.
    variables_in_heads = set()
    literals_in_heads = set()
    variables_with_layers = set()

    for axiom in axioms:
        head = axiom.effect
        variables_in_heads.add(head.positive())
        literals_in_heads.add(head)
    variables_with_layers = set(axiom_layers.keys())

    # 1. Each derived variable only appears in heads with one
    #    polarity, i.e., never positively *and* negatively.
    if False:
        print("Verifying 1...")
        for literal in literals_in_heads:
            assert literal.negate() not in literals_in_heads, literal
    else:
        print("Verifying 1... [skipped]")
        # We currently violate this condition because we introduce
        # "negated axioms". See issue454 and issue453.

    # 2. A variable has a defined layer iff it appears in a head.
    #    (This is stricter than it needs to be; we could allow
    #    derived variables that are never generated by a rule.
    #    But this test follows the axiom simplification step, and
    #    after simplification this should not be too strict.)
    #    All layers are integers and at least 0.
    #    (Note: the "-1" layer for non-derived variables is
    #    set elsewhere.)
    print("Verifying 2...")
    assert variables_in_heads == variables_with_layers
    for atom, layer in axiom_layers.items():
        assert isinstance(layer, int)
        assert layer >= 0

    # 3. For every derived variable, it occurs in axiom_init iff
    #    its negation occurs as the head of an axiom.
    if False:
        print("Verifying 3...")
        for init in list(axiom_init):
            assert init.negate() in literals_in_heads
        for literal in literals_in_heads:
            assert (literal.negated) == (literal.positive() in axiom_init)
    else:
        print("Verifying 3 [weaker version]...")
        # We currently violate this condition because we introduce
        # "negated axioms". See issue454 and issue453.
        #
        # The weaker version we test here is "For every derived variable:
        # [it occurs in axiom_init iff its negation occurs as the
        # head of an axiom] OR [it occurs with both polarities in
        # heads of axioms]."
        for init in list(axiom_init):
            assert init.negate() in literals_in_heads
        for literal in literals_in_heads:
            assert ((literal.negated) == (literal.positive() in axiom_init)
                    or (literal.negate() in literals_in_heads))

    # 4. For every rule head <- ... cond ... where cond is a literal
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
    print("Verifying 4...")
    for axiom in axioms:
        head = axiom.effect
        head_positive = head.positive()
        body = axiom.condition
        for cond in body:
            cond_positive = cond.positive()
            if (cond_positive in variables_in_heads and
                axiom_layers[cond_positive] == axiom_layers[head_positive]):
                assert cond in literals_in_heads

    # 5. For every rule head <- ... cond ... where cond is a literal
    #    of a derived variable, the layer of head is greater or equal
    #    to the layer of cond.
    print("Verifying 5...")
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


def compute_reversed_topological_sort(clusters):
    sorted_clusters = []
    temporarily_marked = set()
    permanently_marked = set()
    
    def visit(cluster):
        if cluster in permanently_marked:
            return
        assert cluster not in temporarily_marked
        temporarily_marked.add(cluster)
        for child in cluster.positive_children.union(cluster.negative_children):
            visit(child)
        temporarily_marked.remove(cluster)
        permanently_marked.add(cluster)
        sorted_clusters.append(cluster)
        
    for cluster in clusters:
        if cluster not in permanently_marked:
            visit(cluster)
    return sorted_clusters


def compute_single_cluster_layer(cluster):
    # TODO: discuss with Malte if this is the right way to calculate layers
    layers = [0]
    for pos_child in cluster.positive_children:
        if pos_child.inverted == cluster.inverted:
            layers.append(pos_child.layer)
        else:
            layers.append(pos_child.layer + 1)
    for neg_child in cluster.negative_children:
        if neg_child.inverted == cluster.inverted:
            layers.append(neg_child.layer + 1)
        else:
            layers.append(neg_child.layer)
    return max(layers)


def compute_axiom_layers(clusters, strategy):
    reversed_clusters = compute_reversed_topological_sort(clusters)
    
    layers = defaultdict(int)
    if strategy == "max":
        layer = 0
        for cluster in reversed_clusters:
            for variable in cluster.variables:
                 layers[variable] = layer
            layer += 1
    elif strategy == "min":
        for cluster in reversed_clusters:
            cluster.layer = compute_single_cluster_layer(cluster)
            for variable in cluster.variables:
                layers[variable] = cluster.layer
    return layers


def simplify_axioms(clusters):
    for cluster in clusters:
        for variable in cluster.variables:
            simplify(cluster.axioms[variable])


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
    axioms_by_literal = {}
    for axiom in axioms:
        if axiom.effect in axiom.condition:
            axioms_to_skip.add(id(axiom))
        else:
            for literal in axiom.condition:
                axioms_by_literal.setdefault(literal, set()).add(id(axiom))

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


def negate_overapproximation(variable):
    """Compute axioms for a negative derived predicate 'd' using a dummy
    rule True -> d.
    """
    axiom_name = str(variable)
    result = pddl.PropositionalAxiom(axiom_name, [], variable.negate())
    return [result]


def compute_negative_axioms(clusters):
    axiom_init = []
    for cluster in clusters:
        if cluster.needed_negative:
            atomic_cluster = len(cluster.variables) > 1
            for variable in cluster.variables:
                if atomic_cluster:
                    negated_axioms = negate_overapproximation(variable)
                else:
                    negated_axioms = negate(cluster.axioms[variable])

                cluster.axioms[variable] += negated_axioms
    return axiom_init


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
        elif len(condition) == 1: # Handle easy special case quickly.
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
