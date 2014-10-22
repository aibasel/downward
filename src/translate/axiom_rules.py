import pddl
import timers

def handle_axioms(operators, axioms, goals):
    axioms_by_atom = get_axioms_by_atom(axioms)

    axiom_literals = compute_necessary_axiom_literals(axioms_by_atom, operators, goals)
    axiom_init = get_axiom_init(axioms_by_atom, axiom_literals)
    with timers.timing("Simplifying axioms"):
        axioms = simplify_axioms(axioms_by_atom, axiom_literals)
    axioms = compute_negative_axioms(axioms_by_atom, axiom_literals)
    # NOTE: compute_negative_axioms more or less invalidates axioms_by_atom.
    #       Careful with that axe, Eugene!
    axiom_layers = compute_axiom_layers(axioms, axiom_init)
    return axioms, list(axiom_init), axiom_layers

def get_axioms_by_atom(axioms):
    axioms_by_atom = {}
    for axiom in axioms:
        axioms_by_atom.setdefault(axiom.effect, []).append(axiom)
    return axioms_by_atom

def compute_axiom_layers(axioms, axiom_init):
    NO_AXIOM = -1
    UNKNOWN_LAYER = -2
    FIRST_MARKER = -3

    depends_on = {}
    for axiom in axioms:
        effect_atom = axiom.effect.positive()
        effect_sign = not axiom.effect.negated
        effect_init_sign = effect_atom in axiom_init
        if effect_sign != effect_init_sign:
            depends_on.setdefault(effect_atom, set())
            for condition in axiom.condition:
                condition_atom = condition.positive()
                condition_sign = not condition.negated
                condition_init_sign = condition_atom in axiom_init
                if condition_sign == condition_init_sign:
                    depends_on[effect_atom].add((condition_atom, +1))
                else:
                    depends_on[effect_atom].add((condition_atom, +0))

    layers = dict([(atom, UNKNOWN_LAYER) for atom in depends_on])
    def find_level(atom, marker):
        layer = layers.get(atom, NO_AXIOM)
        if layer == NO_AXIOM:
            return 0

        if layer == marker:
            # Found positive cycle: May return 0 but not set value.
            return 0
        elif layer <= FIRST_MARKER:
            # Found negative cycle: Error.
            assert False, "Cyclic dependencies in axioms; cannot stratify."
        if layer == UNKNOWN_LAYER:
            layers[atom] = marker
            layer = 0
            for (condition_atom, bonus) in depends_on[atom]:
                layer = max(layer, find_level(condition_atom, marker - bonus) + bonus)
            layers[atom] = layer
        return layer
    for atom in depends_on:
        find_level(atom, FIRST_MARKER)

    #for atom, layer in layers.iteritems():
    #  print "Layer %d: %s" % (layer, atom)
    return layers

def compute_necessary_axiom_literals(axioms_by_atom, operators, goal):
    necessary_literals = set()
    queue = []

    def register_literals(literals, negated):
        for literal in literals:
            if literal.positive() in axioms_by_atom:   # This is an axiom literal
                if negated:
                    literal = literal.negate()
                if literal not in necessary_literals:
                    necessary_literals.add(literal)
                    queue.append(literal)

    # Initialize queue with axioms required for goal and operators.
    register_literals(goal, False)
    for op in operators:
        register_literals(op.precondition, False)
        for (cond, _) in op.add_effects:
            register_literals(cond, False)
        for (cond, _) in op.del_effects:
            register_literals(cond, True)

    while queue:
        literal = queue.pop()
        axioms = axioms_by_atom[literal.positive()]
        for axiom in axioms:
            register_literals(axiom.condition, literal.negated)
    return necessary_literals

def get_axiom_init(axioms_by_atom, necessary_literals):
    result = set()
    for atom in axioms_by_atom:
        if atom not in necessary_literals and atom.negate() in necessary_literals:
            # Initial value for axiom: False (which is omitted due to closed world
            # assumption) unless it is only needed negatively.
            result.add(atom)
    return result

def simplify_axioms(axioms_by_atom, necessary_literals):
    necessary_atoms = set([literal.positive() for literal in necessary_literals])
    new_axioms = []
    for atom in necessary_atoms:
        axioms = simplify(axioms_by_atom[atom])
        axioms_by_atom[atom] = axioms
        new_axioms += axioms
    return new_axioms

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
    axioms_by_literal = {}
    for axiom in axioms:
        for literal in axiom.condition:
            axioms_by_literal.setdefault(literal, set()).add(id(axiom))

    axioms_to_skip = set()
    for axiom in axioms:
        if id(axiom) in axioms_to_skip:
            continue   # Required to keep one of multiple identical axioms.
        if not axiom.condition: # empty condition: dominates everything
            return [axiom]
        literals = iter(axiom.condition)
        dominated_axioms = axioms_by_literal[next(literals)]
        for literal in literals:
            dominated_axioms &= axioms_by_literal[literal]
        for dominated_axiom in dominated_axioms:
            if dominated_axiom != id(axiom):
                axioms_to_skip.add(dominated_axiom)
    return [axiom for axiom in axioms if id(axiom) not in axioms_to_skip]

def compute_negative_axioms(axioms_by_atom, necessary_literals):
    new_axioms = []
    for literal in necessary_literals:
        if literal.negated:
            new_axioms += negate(axioms_by_atom[literal.positive()])
        else:
            new_axioms += axioms_by_atom[literal]
    return new_axioms

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
