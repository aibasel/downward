import contextlib
import sys

import graph
import pddl
from .parse_error import ParseError


SYNTAX_LITERAL = "(PREDICATE ARGUMENTS*)"
SYNTAX_LITERAL_NEGATED = "(not (PREDICATE ARGUMENTS*))"
SYNTAX_LITERAL_POSSIBLY_NEGATED = f"{SYNTAX_LITERAL} or {SYNTAX_LITERAL_NEGATED}"

SYNTAX_PREDICATE = "(PREDICATE_NAME [VARIABLE [- TYPE]?]*)"
SYNTAX_PREDICATES = f"(:predicates {SYNTAX_PREDICATE}*)"
SYNTAX_FUNCTION = "(FUNCTION_NAME [VARIABLE [- TYPE]?]*)"
SYNTAX_ACTION = "(:action NAME [:parameters PARAMETERS]? " \
                "[:precondition PRECONDITION]? :effect EFFECT)"
SYNTAX_AXIOM = "(:derived PREDICATE CONDITION)"
SYNTAX_GOAL = "(:goal GOAL)"

SYNTAX_CONDITION_AND = "(and CONDITION*)"
SYNTAX_CONDITION_OR = "(or CONDITION*)"
SYNTAX_CONDITION_IMPLY = "(imply CONDITION CONDITION)"
SYNTAX_CONDITION_NOT = "(not CONDITION)"
SYNTAX_CONDITION_FORALL_EXISTS = "({forall, exists} VARIABLES CONDITION)"

SYNTAX_EFFECT_FORALL = "(forall VARIABLES EFFECT)"
SYNTAX_EFFECT_WHEN = "(when CONDITION EFFECT)"
SYNTAX_EFFECT_INCREASE = "(increase (total-cost) ASSIGNMENT)"

SYNTAX_EXPRESSION = "POSITIVE_NUMBER or (FUNCTION VARIABLES*)"
SYNTAX_ASSIGNMENT = "({=,increase} EXPRESSION EXPRESSION)"

SYNTAX_DOMAIN_DOMAIN_NAME = "(domain NAME)"
SYNTAX_TASK_PROBLEM_NAME = "(problem NAME)"
SYNTAX_TASK_DOMAIN_NAME = "(:domain NAME)"
SYNTAX_METRIC = "(:metric minimize (total-cost))"


CONDITION_TAG_TO_SYNTAX = {
    "and": SYNTAX_CONDITION_AND,
    "or": SYNTAX_CONDITION_OR,
    "imply": SYNTAX_CONDITION_IMPLY,
    "not": SYNTAX_CONDITION_NOT,
    "forall": SYNTAX_CONDITION_FORALL_EXISTS,
}


class Context:
    def __init__(self):
        self._traceback = []

    def __str__(self) -> str:
        return "\n\t->".join(self._traceback)

    def error(self, message, item=None, syntax=None):
        error_msg = f"{self}\n{message}"
        if syntax:
            error_msg += f"\nSyntax: {syntax}"
        if item:
            error_msg += f"\nGot: {item}"
        raise ParseError(error_msg)

    def expected_word_error(self, name, *args, **kwargs):
        self.error(f"{name} is expected to be a word.", *args, **kwargs)

    def expected_list_error(self, name, *args, **kwargs):
        self.error(f"{name} is expected to be a block.", *args, **kwargs)

    def expected_named_block_error(self, alist, expected, *args, **kwargs):
        self.error(f"Expected a non-empty block starting with any of the "
                   f"following words: {', '.join(expected)}",
                   item=alist, *args, **kwargs)

    @contextlib.contextmanager
    def layer(self, message: str):
        self._traceback.append(message)
        yield
        assert self._traceback.pop() == message


def check_named_block(alist, names):
    return isinstance(alist, list) and alist and alist[0] in names

def assert_named_block(context, alist, names):
    if not check_named_block(alist, names):
        context.expected_named_block_error(alist, names)

def construct_typed_object(context, name, _type):
    with context.layer("Parsing typed object"):
        return pddl.TypedObject(name, _type)


def construct_type(context, curr_type, base_type):
    with context.layer("Parsing PDDL type"):
        return pddl.Type(curr_type, base_type)


def parse_typed_list(context, alist, only_variables=False,
                     constructor=construct_typed_object,
                     default_type="object"):
    with context.layer("Parsing typed list"):
        result = []
        while alist:
            with context.layer(f"Parsing {group_number}. group of typed list"):
                try:
                    separator_position = alist.index("-)
                except ValueError:
                    items = alist
                    _type = default_type
                    alist = []
                else:
                    items = alist[:separator_position]
                    _type = alist[separator_position + 1]
                    alist = alist[separator_position + 2:]
                for item in items:
                    assert not only_variables or item.startswith("?"), \
                           "Expected item to be a variable: %s in (%s)" % (
                        item, " ".join(items))
                    entry = constructor(context, item, _type)
                    result.append(entry)
        return result


def set_supertypes(type_list):
    # TODO: This is a two-stage construction, which is perhaps
    # not a great idea. Might need more thought in the future.
    type_name_to_type = {}
    child_types = []
    for type in type_list:
        type.supertype_names = []
        type_name_to_type[type.name] = type
        if type.basetype_name:
            child_types.append((type.name, type.basetype_name))
    for (desc_name, anc_name) in graph.transitive_closure(child_types):
        type_name_to_type[desc_name].supertype_names.append(anc_name)


def parse_requirements(context, alist):
    with context.layer("Parsing requirements"):
        try:
            return pddl.Requirements(alist)
        except ValueError as e:
            context.error(f"Error in requirements.\n"
                          f"Reason: {e}")


def parse_predicate(context, alist):
    with context.layer("Parsing predicate name"):
        name = alist[0]
    with context.layer(f"Parsing arguments of predicate '{name}'"):
        arguments = parse_typed_list(context, alist[1:], only_variables=True)
    return pddl.Predicate(name, arguments)


def parse_predicates(context, alist):
    with context.layer("Parsing predicates"):
        the_predicates = []
        for no, entry in enumerate(alist):
            with context.layer(f"Parsing {no}. predicate"):
                if not isinstance(entry, list):
                the_predicates.append(parse_predicate(context, entry))
        return the_predicates


def parse_function(context, alist, type_name):
    with context.layer("Parsing function name"):
        name = alist[0]
    with context.layer(f"Parsing function '{name}'"):
        arguments = parse_typed_list(context, alist[1:])
    return pddl.Function(name, arguments, type_name)


def parse_condition(context, alist, type_dict, predicate_dict):
    with context.layer("Parsing condition"):
        condition = parse_condition_aux(context, alist, False, type_dict, predicate_dict)
        return condition.uniquify_variables({}).simplified()


def parse_condition_aux(context, alist, negated, type_dict, predicate_dict):
    """Parse a PDDL condition. The condition is translated into NNF on the fly."""
    tag = alist[0]
    if tag in ("and", "or", "not", "imply"):
        args = alist[1:]
        if tag == "imply":
            assert len(args) == 2
        if tag == "not":
            assert len(args) == 1
            negated = not negated
    elif tag in ("forall", "exists"):
        parameters = parse_typed_list(context, alist[1])
        args = alist[2:]
        assert len(args) == 1
    elif tag in predicate_dict:
        return parse_literal(context, alist, type_dict, predicate_dict, negated=negated)
    else:
        context.error(f"Expected logical operator or predicate name", tag)

    if tag == "imply":
        parts = [parse_condition_aux(
                context, args[0], not negated, type_dict, predicate_dict),
                 parse_condition_aux(
                context, args[1], negated, type_dict, predicate_dict)]
        tag = "or"
    else:
        parts = [parse_condition_aux(context, part, negated, type_dict, predicate_dict)
                 for part in args]

    if tag == "and" and not negated or tag == "or" and negated:
        return pddl.Conjunction(parts)
    elif tag == "or" and not negated or tag == "and" and negated:
        return pddl.Disjunction(parts)
    elif tag == "forall" and not negated or tag == "exists" and negated:
        return pddl.UniversalCondition(parameters, parts)
    elif tag == "exists" and not negated or tag == "forall" and negated:
        return pddl.ExistentialCondition(parameters, parts)
    elif tag == "not":
        return parts[0]


def parse_literal(context, alist, type_dict, predicate_dict, negated=False):
    with context.layer("Parsing literal"):
        if alist[0] == "not":
            assert len(alist) == 2
            alist = alist[1]
            negated = not negated

        pred_id, arity = _get_predicate_id_and_arity(
            context, alist[0], type_dict, predicate_dict)

        if arity != len(alist) - 1:
            context.error(f"Predicate '{predicate_name}' of arity {arity} used"
                          f" with {len(alist) -1} arguments.", alist)

        if negated:
            return pddl.NegatedAtom(pred_id, alist[1:])
        else:
            return pddl.Atom(pred_id, alist[1:])


SEEN_WARNING_TYPE_PREDICATE_NAME_CLASH = False
def _get_predicate_id_and_arity(context, text, type_dict, predicate_dict):
    global SEEN_WARNING_TYPE_PREDICATE_NAME_CLASH

    the_type = type_dict.get(text)
    the_predicate = predicate_dict.get(text)

    if the_type is None and the_predicate is None:
        context.error(f"Undeclared predicate", text)
    elif the_predicate is not None:
        if the_type is not None and not SEEN_WARNING_TYPE_PREDICATE_NAME_CLASH:
            msg = ("Warning: name clash between type and predicate %r.\n"
                   "Interpreting as predicate in conditions.") % text
            print(msg, file=sys.stderr)
            SEEN_WARNING_TYPE_PREDICATE_NAME_CLASH = True
        return the_predicate.name, the_predicate.get_arity()
    else:
        assert the_type is not None
        return the_type.get_predicate_name(), 1


def parse_effects(context, alist, result, type_dict, predicate_dict):
    """Parse a PDDL effect (any combination of simple, conjunctive, conditional, and universal)."""
    with context.layer("Parsing effect"):
        tmp_effect = parse_effect(context, alist, type_dict, predicate_dict)
        normalized = tmp_effect.normalize()
        cost_eff, rest_effect = normalized.extract_cost()
        add_effect(rest_effect, result)
        if cost_eff:
            return cost_eff.effect
        else:
            return None

def add_effect(tmp_effect, result):
    """tmp_effect has the following structure:
       [ConjunctiveEffect] [UniversalEffect] [ConditionalEffect] SimpleEffect."""

    if isinstance(tmp_effect, pddl.ConjunctiveEffect):
        for effect in tmp_effect.effects:
            add_effect(effect, result)
        return
    else:
        parameters = []
        condition = pddl.Truth()
        if isinstance(tmp_effect, pddl.UniversalEffect):
            parameters = tmp_effect.parameters
            if isinstance(tmp_effect.effect, pddl.ConditionalEffect):
                condition = tmp_effect.effect.condition
                assert isinstance(tmp_effect.effect.effect, pddl.SimpleEffect)
                effect = tmp_effect.effect.effect.effect
            else:
                assert isinstance(tmp_effect.effect, pddl.SimpleEffect)
                effect = tmp_effect.effect.effect
        elif isinstance(tmp_effect, pddl.ConditionalEffect):
            condition = tmp_effect.condition
            assert isinstance(tmp_effect.effect, pddl.SimpleEffect)
            effect = tmp_effect.effect.effect
        else:
            assert isinstance(tmp_effect, pddl.SimpleEffect)
            effect = tmp_effect.effect
        assert isinstance(effect, pddl.Literal)
        # Check for contradictory effects
        condition = condition.simplified()
        new_effect = pddl.Effect(parameters, condition, effect)
        contradiction = pddl.Effect(parameters, condition, effect.negate())
        if contradiction not in result:
            result.append(new_effect)
        else:
            # We use add-after-delete semantics, keep positive effect
            if isinstance(contradiction.literal, pddl.NegatedAtom):
                result.remove(contradiction)
                result.append(new_effect)


def parse_effect(context, alist, type_dict, predicate_dict):
    tag = alist[0]
    if tag == "and":
        return pddl.ConjunctiveEffect(
            [parse_effect(eff, type_dict, predicate_dict) for eff in alist[1:]])
    elif tag == "forall":
        assert len(alist) == 3
        parameters = parse_typed_list(context, alist[1])
        effect = parse_effect(context, alist[2], type_dict, predicate_dict)
        return pddl.UniversalEffect(parameters, effect)
    elif tag == "when":
        assert len(alist) == 3
        condition = parse_condition(context, alist[1], type_dict, predicate_dict)
        effect = parse_effect(context, alist[2], type_dict, predicate_dict)
        return pddl.ConditionalEffect(condition, effect)
    elif tag == "increase":
        assert len(alist) == 3
        assert alist[1] == ['total-cost']
        assignment = parse_assignment(context, alist)
        return pddl.CostEffect(assignment)
    else:
        # We pass in {} instead of type_dict here because types must
        # be static predicates, so cannot be the target of an effect.
        return pddl.SimpleEffect(parse_literal(context, alist, {}, predicate_dict))


def parse_expression(context, exp):
    with context.layer("Parsing expression"):
        if isinstance(exp, list):
            functionsymbol = exp[0]
            return pddl.PrimitiveNumericExpression(functionsymbol, exp[1:])
        elif exp.replace(".", "").isdigit():
            return pddl.NumericConstant(float(exp))
        elif exp[0] == "-":
            context.error("Expression cannot be a negative number",
                          syntax=SYNTAX_EXPRESSION)
        else:
            return pddl.PrimitiveNumericExpression(exp, [])


def parse_assignment(context, alist):
    with context.layer("Parsing Assignment"):
        assert len(alist) == 3
        op = alist[0]
        head = parse_expression(context, alist[1])
        exp = parse_expression(context, alist[2])
        if op == "=":
            return pddl.Assign(head, exp)
        elif op == "increase":
            return pddl.Increase(head, exp)
        else:
            context.error(f"Unsupported assignment operator '{op}'."
                          f" Use '=' or 'increase'.")


def parse_action(context, alist, type_dict, predicate_dict):
    with context.layer("Parsing action name"):
        iterator = iter(alist)
        action_tag = next(iterator)
        assert action_tag == ":action"
        name = next(iterator)
    with context.layer(f"Parsing action '{name}'"):
        try:
            with context.layer("Parsing parameters"):
                parameters_tag_opt = next(iterator)
                if parameters_tag_opt == ":parameters":
                    parameters = parse_typed_list(next(iterator),
                                      only_variables=True)
                    precondition_tag_opt = next(iterator)
                else:
                    parameters = []
                    precondition_tag_opt = parameters_tag_opt
            with context.layer("Parsing precondition"):
                if precondition_tag_opt == ":precondition":
                    precondition_list = next(iterator)
                    if not precondition_list:
                        # Note that :precondition () is allowed in PDDL.
                        precondition = pddl.Conjunction([])
                    else:
                        precondition = parse_condition(
                            context, precondition_list, type_dict, predicate_dict)
                    effect_tag = next(iterator)
                else:
                    precondition = pddl.Conjunction([])
                    effect_tag = precondition_tag_opt
            with context.layer("Parsing effect"):
                assert effect_tag == ":effect"
                effect_list = next(iterator)
                eff = []
                if effect_list:
                    cost = parse_effects(
                        context, effect_list, eff, type_dict, predicate_dict)
        except StopIteration:
            context.error(f"Missing fields. Expecting {SYNTAX_ACTION}.")
        for _ in iterator:
            context.error(f"Too many fields. Expecting {SYNTAX_ACTION}")
    if eff:
        return pddl.Action(name, parameters, len(parameters),
                           precondition, eff, cost)
    else:
        return None


def parse_axiom(context, alist, type_dict, predicate_dict):
    with context.layer("Parsing derived predicate"):
        assert len(alist) == 3
        assert alist[0] == ":derived"
        predicate = parse_predicate(context, alist[1])
    with context.layer(f"Parsing condition for derived predicate '{predicate}'"):
        condition = parse_condition(
            context, alist[2], type_dict, predicate_dict)
        return pddl.Axiom(predicate.name, predicate.arguments,
                          len(predicate.arguments), condition)


def parse_axioms_and_actions(context, entries, type_dict, predicate_dict):
    the_axioms = []
    the_actions = []
    for no, entry in enumerate(entries, start=1):
        with context.layer(f"Parsing {no}. axiom/action entry"):
            if entry[0] == ":derived":
                with context.layer(f"Parsing {len(the_axioms) + 1}. axiom"):
                    the_axioms.append(parse_axiom(
                        context, entry, type_dict, predicate_dict))
            else:
                assert entry[0] == ":action"
                with context.layer(f"Parsing {len(the_actions) + 1}. action"):
                    action = parse_action(context, entry, type_dict, predicate_dict)
                    if action is not None:
                        the_actions.append(action)
    return the_axioms, the_actions

def parse_init(context, alist):
    initial = []
    initial_true = set()
    initial_false = set()
    initial_assignments = dict()
    for no, fact in enumerate(alist[1:], start=1):
        with context.layer(f"Parsing {no}. element in init block"):
            if fact[0] == "=":
                try:
                    assignment = parse_assignment(context, fact)
                except ValueError as e:
                    context.error(f"Error in initial state specification\n"
                                  f"Reason: {e}.")
                if not isinstance(assignment.expression,
                                  pddl.NumericConstant):
                    context.error("Illegal assignment in initial state specification.",
                                  assignment)
                if assignment.fluent in initial_assignments:
                    prev = initial_assignments[assignment.fluent]
                    if assignment.expression == prev.expression:
                        print(f"Warning: {assignment} is specified twice "
                              f"in initial state specification")
                    else:
                        context.error("Error in initial state specification\n"
                                      "Reason: conflicting assignment for "
                                      f"{assignment.fluent}.")
                else:
                    initial_assignments[assignment.fluent] = assignment
                    initial.append(assignment)
            elif fact[0] == "not":
                fact = fact[1]
                atom = pddl.Atom(fact[0], fact[1:])
                check_atom_consistency(context, atom, initial_false, initial_true, False)
                initial_false.add(atom)
            else:
                if len(fact) < 1:
                    context.error(f"Expecting {SYNTAX_LITERAL} for atoms.")
                atom = pddl.Atom(fact[0], fact[1:])
                check_atom_consistency(context, atom, initial_true, initial_false)
                initial_true.add(atom)
        initial.extend(initial_true)
    return initial


def parse_task(domain_pddl, task_pddl):
    context = Context()
    domain_name, domain_requirements, types, type_dict, constants, predicates, \
        predicate_dict, functions, actions, axioms = parse_domain_pddl(context, domain_pddl)
    task_name, task_domain_name, task_requirements, objects, init, goal, \
        use_metric = parse_task_pddl(context, task_pddl, type_dict, predicate_dict)

    if domain_name != task_domain_name:
        context.error(f"The domain name specified by the task "
                      f"({task_domain_name}) does not match the name specified "
                      f"by the domain file ({domain_name}).")
    requirements = pddl.Requirements(sorted(set(
                domain_requirements.requirements +
                task_requirements.requirements)))
    objects = constants + objects
    check_for_duplicates(
        context,
        [o.name for o in objects],
        errmsg="error: duplicate object %r",
        finalmsg="please check :constants and :objects definitions")
    init += [pddl.Atom("=", (obj.name, obj.name)) for obj in objects]

    return pddl.Task(
        domain_name, task_name, requirements, types, objects,
        predicates, functions, init, goal, actions, axioms, use_metric)


def parse_domain_pddl(context, domain_pddl):
    iterator = iter(domain_pddl)
    with context.layer("Parsing domain"):
        define_tag = next(iterator)
        if define_tag != "define":
            context.error(f"Domain definition expected to start with '(define '. Got '({define_tag}'")

        with context.layer("Parsing domain name"):
            domain_line = next(iterator)
            if (not check_named_block(domain_line, ["domain"]) or
                    len(domain_line) != 2 or not isinstance(domain_line[1], str)):
                context.error(f"Invalid definition of domain name.",
                              syntax=SYNTAX_DOMAIN_DOMAIN_NAME)
            yield domain_line[1]

        ## We allow an arbitrary order of the requirement, types, constants,
        ## predicates and functions specification. The PDDL BNF is more strict on
        ## this, so we print a warning if it is violated.
        requirements = pddl.Requirements([":strips"])
        the_types = [pddl.Type("object")]
        constants, the_predicates, the_functions = [], [], []
        correct_order = [":requirements", ":types", ":constants", ":predicates",
                         ":functions"]
        seen_fields = []
        first_action = None
        for opt in iterator:
            field = opt[0]
            if field not in correct_order:
                first_action = opt
                break
            if field in seen_fields:
                context.error(f"Error in domain specification\n"
                              f"Reason: two '{field}' specifications.")
            if (seen_fields and
                correct_order.index(seen_fields[-1]) > correct_order.index(field)):
                msg = f"\nWarning: {field} specification not allowed here (cf. PDDL BNF)"
                print(msg, file=sys.stderr)
            seen_fields.append(field)
            if field == ":requirements":
                requirements = parse_requirements(context, opt[1:])
            elif field == ":types":
                with context.layer("Parsing types"):
                    the_types.extend(parse_typed_list(
                            context, opt[1:], constructor=construct_type))
            elif field == ":constants":
                with context.layer("Parsing constants"):
                    constants = parse_typed_list(context, opt[1:])
            elif field == ":predicates":
                the_predicates = parse_predicates(context, opt[1:])
                the_predicates += [pddl.Predicate("=", [
                    pddl.TypedObject("?x", "object"),
                    pddl.TypedObject("?y", "object")])]
            elif field == ":functions":
                with context.layer("Parsing functions"):
                    the_functions = parse_typed_list(
                        context, opt[1:],
                        constructor=parse_function,
                        default_type="number")
        set_supertypes(the_types)
        yield requirements
        yield the_types
        type_dict = {type.name: type for type in the_types}
        yield type_dict
        yield constants
        yield the_predicates
        predicate_dict = {pred.name: pred for pred in the_predicates}
        yield predicate_dict
        yield the_functions

        entries = []
        if first_action is not None:
            entries.append(first_action)
        entries.extend(iterator)

        the_axioms, the_actions = parse_axioms_and_actions(
            context, entries, type_dict, predicate_dict)

        yield the_actions
        yield the_axioms

def parse_task_pddl(context, task_pddl, type_dict, predicate_dict):
    iterator = iter(task_pddl)
    with context.layer("Parsing task"):
        define_tag = next(iterator)
        if define_tag != "define":
            context.error("Task definition expected to start with '(define ")

        with context.layer("Parsing problem name"):
            problem_line = next(iterator)
            if (not check_named_block(problem_line, ["problem"]) or
                    len(problem_line) != 2 or not isinstance(problem_line[1], str)):
                context.error("Invalid problem name definition", problem_line,
                              syntax=SYNTAX_TASK_PROBLEM_NAME)
            yield problem_line[1]

        with context.layer("Parsing domain name"):
            domain_line = next(iterator)
            if (not check_named_block(domain_line, [":domain"]) or
                    len(domain_line) != 2 or not isinstance(domain_line[1], str)):
                context.error("Invalid domain name definition", domain_line,
                              syntax=SYNTAX_TASK_DOMAIN_NAME)
            yield domain_line[1]

        requirements_opt = next(iterator)
        if requirements_opt[0] == ":requirements":
            requirements = requirements_opt[1:]
            objects_opt = next(iterator)
        else:
            requirements = []
            objects_opt = requirements_opt
        yield parse_requirements(context, requirements)

        if objects_opt[0] == ":objects":
            with context.layer("Parsing objects"):
                yield parse_typed_list(context, objects_opt[1:])
            init = next(iterator)
        else:
            yield []
            init = objects_opt

        assert init[0] == ":init"
        yield parse_init(context, init)

        goal = next(iterator)
        with context.layer("Parsing goal"):
            if (not check_named_block(goal, [":goal"]) or
                    len(goal) != 2 or not isinstance(goal[1], list) or
                    not goal[1]):
                context.error("Expected non-empty goal.", syntax=SYNTAX_GOAL)
            yield parse_condition(context, goal[1], type_dict, predicate_dict)

        use_metric = False
        for entry in iterator:
        if entry[0] == ":metric":
            if entry[1] == "minimize" and entry[2][0] == "total-cost":
                use_metric = True
            else:
                assert False, "Unknown metric."
        yield use_metric

        for _ in iterator:
            context.error("No blocks expected after goal and metric.")


def check_atom_consistency(context, atom, same_truth_value, other_truth_value, atom_is_true=True):
    if atom in other_truth_value:
        context.error(f"Error in initial state specification\n"
                      f"Reason: {atom} is true and false.")
    if atom in same_truth_value:
        if not atom_is_true:
            atom = atom.negate()
        print(f"Warning: {atom} is specified twice in initial state specification")

def check_for_duplicates(context, elements, errmsg, finalmsg):
    seen = set()
    errors = []
    for element in elements:
        if element in seen:
            errors.append(errmsg % element)
        else:
            seen.add(element)
    if errors:
        context.error("\n".join(errors) + "\n" + finalmsg)
