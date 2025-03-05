import contextlib
import sys

import graph
import pddl
from .warning import print_warning
from .parse_error import ParseError

TYPED_LIST_SEPARATOR = "-"

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
        if item is not None:
            error_msg += f"\nGot: {lispify(item)}"
        raise ParseError(error_msg)

    @contextlib.contextmanager
    def layer(self, message: str):
        self._traceback.append(message)
        yield
        assert self._traceback.pop() == message

def lispify(item):
    if isinstance(item, list):
        return "(" + " ".join([lispify(i) for i in item]) + ")"
    else:
        return item

def check_word(context, word, description, syntax=None):
    if not isinstance(word, str):
        context.error(f"{description} is expected to be a word.", item=word, syntax=syntax)


def check_list(context, alist, description, syntax=None):
    if not isinstance(alist, list):
        context.error(f"{description} is expected to be a block.", item=alist, syntax=syntax)


def check_named_block(context, alist, names, syntax=None):
    if not (isinstance(alist, list) and alist and alist[0] in names):
        context.error(f"Expected a non-empty block starting with any of the "
                      f"following words: {', '.join(names)}",
                      item=alist, syntax=syntax)


def construct_typed_object(context, name, _type):
    with context.layer("Parsing typed object"):
        check_word(context, name, "Name of typed object")
        # We don't check the type here because in the predicates definition
        # we allow using "either", making the type a list and not a word.
        return pddl.TypedObject(name, _type)


def construct_type(context, curr_type, base_type):
    with context.layer("Parsing PDDL type"):
        check_word(context, curr_type, "PDDL type")
        check_word(context, base_type, "Base type")
        return pddl.Type(curr_type, base_type)


def parse_typed_list(context, alist, only_variables=False, either_allowed=False,
                     constructor=construct_typed_object,
                     default_type="object"):
    with context.layer("Parsing typed list"):
        result = []
        group_number = 1
        while alist:
            with context.layer(f"Parsing {group_number}. group of typed list"):
                try:
                    separator_position = alist.index(TYPED_LIST_SEPARATOR)
                except ValueError:
                    items = alist
                    _type = default_type
                    alist = []
                else:
                    if separator_position == len(alist) - 1:
                        context.error(
                            f"Type missing after '{TYPED_LIST_SEPARATOR}'.",
                            alist)
                    items = alist[:separator_position]
                    if not items:
                        print_warning(f"Expected something before the separator '{TYPED_LIST_SEPARATOR}'."
                                      f" Got: {lispify(alist)}")
                    _type = alist[separator_position + 1]
                    alist = alist[separator_position + 2:]
                    if not isinstance(_type, str):
                        if not either_allowed:
                            context.error("Type value is expected to be a single word.", _type)
                        elif not (_type and _type[0] == "either"):
                            context.error("Type value is expected to be a single word "
                                          "or '(either WORD*)'", _type)
                for item in items:
                    if only_variables and not item.startswith("?"):
                        context.error("Expected a variable but the given word does not start with '?'.", item)
                    entry = constructor(context, item, _type)
                    result.append(entry)
            group_number += 1
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
        for item in alist:
            check_word(context, item, "Requirement label")
        try:
            return pddl.Requirements(alist)
        except ValueError as e:
            context.error(f"Error in requirements.\n"
                          f"Reason: {e}")


def parse_predicate(context, alist):
    with context.layer("Parsing predicate name"):
        if not alist:
            context.error("Predicate name missing", syntax=SYNTAX_PREDICATE)
        name = alist[0]
        check_word(context, name, "Predicate name")
    with context.layer(f"Parsing arguments of predicate '{name}'"):
        arguments = parse_typed_list(
            context, alist[1:], only_variables=True, either_allowed=True)
    return pddl.Predicate(name, arguments)


def parse_predicates(context, alist):
    with context.layer("Parsing predicates"):
        the_predicates = []
        for no, entry in enumerate(alist, start=1):
            with context.layer(f"Parsing predicate #{no}"):
                if not isinstance(entry, list):
                    context.error("Invalid predicate definition.",
                                  syntax=SYNTAX_PREDICATE)
                the_predicates.append(parse_predicate(context, entry))
        return the_predicates


def parse_function(context, alist, type_name):
    with context.layer("Parsing function name"):
        if not isinstance(alist, list) or len(alist) == 0:
            context.error("Invalid definition of function.",
                          syntax=SYNTAX_FUNCTION)
        name = alist[0]
        check_word(context, name, "Function name")
    with context.layer(f"Parsing function '{name}'"):
        arguments = parse_typed_list(context, alist[1:])
        check_word(context, type_name, "Function type")
    return pddl.Function(name, arguments, type_name)


def parse_condition(context, alist, type_dict, predicate_dict, term_names):
    with context.layer("Parsing condition"):
        condition = parse_condition_aux(
            context, alist, False, type_dict, predicate_dict, term_names)
        return condition.uniquify_variables({}).simplified()


def parse_condition_aux(context, alist, negated, type_dict, predicate_dict, term_names):
    """Parse a PDDL condition. The condition is translated into NNF on the fly."""
    # We allow empty conditions according to "PDDL2.1: An Extension to PDDL for
    # Expressing Temporal Planning Domains" by Maria Fox and Derek Long
    # (JAIR 20:61-124, 2003)
    if not alist:
        return pddl.Conjunction([])
    tag = alist[0]
    if tag in ("and", "or", "not", "imply"):
        args = alist[1:]
        if tag == "imply":
            if len(args) != 2:
                context.error("'imply' expects exactly two arguments.",
                              syntax=SYNTAX_CONDITION_IMPLY)
        if tag == "not":
            if len(args) != 1:
                context.error("'not' expects exactly one argument.",
                              syntax=SYNTAX_CONDITION_NOT)
            negated = not negated
    elif tag in ("forall", "exists"):
        if len(alist) != 3:
            context.error("'forall' and 'exists' expect exactly two arguments.",
                          syntax=SYNTAX_CONDITION_FORALL_EXISTS)
        if not isinstance(alist[1], list) or not alist[1]:
            context.error(
                "The first argument (VARIABLES) of 'forall' and 'exists' is "
                "expected to be a non-empty block.",
                syntax=SYNTAX_CONDITION_FORALL_EXISTS
            )
        parameters = parse_typed_list(context, alist[1])
        args = [alist[2]]
    elif tag in predicate_dict:
        return parse_literal(context, alist, type_dict, predicate_dict, term_names, negated=negated)
    else:
        context.error("Expected logical operator or predicate name", tag)

    for nb_arg, arg in enumerate(args, start=1):
        if not isinstance(arg, list) or not arg:
            context.error(
                f"'{tag}' expects as argument #{nb_arg} a non-empty block.",
                item=arg, syntax=CONDITION_TAG_TO_SYNTAX[tag])

    if tag == "imply":
        parts = [parse_condition_aux(
            context, args[0], not negated, type_dict, predicate_dict, term_names),
            parse_condition_aux(
                context, args[1], negated, type_dict, predicate_dict, term_names)]
        tag = "or"
    else:
        new_term_names = term_names
        if tag in ("forall", "exists"):
            new_term_names = new_term_names | {p.name for p in parameters}
        parts = [parse_condition_aux(
            context, part, negated, type_dict, predicate_dict, new_term_names)
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


def parse_literal(context, alist, type_dict, predicate_dict, term_names, negated=False):
    with context.layer("Parsing literal"):
        if not alist:
            context.error("Literal definition has to be a non-empty block.",
                          alist, syntax=SYNTAX_LITERAL_POSSIBLY_NEGATED)
        if alist[0] == "not":
            if len(alist) != 2:
                context.error(
                    "Negated literal definition has to have exactly one block as argument.",
                    alist, syntax=SYNTAX_LITERAL_NEGATED)
            alist = alist[1]
            if not isinstance(alist, list) or not alist:
                context.error(
                    "Definition of negated literal has to be a non-empty block.",
                    alist, syntax=SYNTAX_LITERAL)
            negated = not negated

        predicate_name = alist[0]
        check_word(context, predicate_name, "Predicate name")
        check_predicate_and_terms_existence(
            context, predicate_name, alist[1:],
            predicate_dict.keys(), term_names)
        pred_id, arity = _get_predicate_id_and_arity(
            context, predicate_name, type_dict, predicate_dict)

        if arity != len(alist) - 1:
            context.error(f"Predicate '{predicate_name}' of arity {arity} used"
                          f" with {len(alist) - 1} arguments.", alist)

        if negated:
            return pddl.NegatedAtom(pred_id, alist[1:])
        else:
            return pddl.Atom(pred_id, alist[1:])


def _get_predicate_id_and_arity(context, text, type_dict, predicate_dict):
    the_type = type_dict.get(text)
    the_predicate = predicate_dict.get(text)

    if the_type is None and the_predicate is None:
        context.error("Undeclared predicate", text)
    elif the_predicate is not None:
        if the_type is not None:
            msg = ("name clash between type and predicate %r.\n"
                   "Interpreting as predicate in conditions.") % text
            print_warning(msg)
        return the_predicate.name, the_predicate.get_arity()
    else:
        assert the_type is not None
        return the_type.get_predicate_name(), 1


def parse_effects(context, alist, result, type_dict, predicate_dict, term_names):
    """Parse a PDDL effect (any combination of simple, conjunctive, conditional, and universal)."""
    with context.layer("Parsing effect"):
        tmp_effect = parse_effect(context, alist, type_dict, predicate_dict, term_names)
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


def parse_effect(context, alist, type_dict, predicate_dict, term_names):
    if not alist:
        context.error("All (sub-)effects have to be a non-empty blocks.", alist)
    tag = alist[0]
    if tag == "and":
        effects = []
        for eff in alist[1:]:
            check_list(context, eff, "Each sub-effect of a conjunction")
            effects.append(parse_effect(context, eff, type_dict, predicate_dict, term_names))
        return pddl.ConjunctiveEffect(effects)
    elif tag == "forall":
        if len(alist) != 3:
            context.error("'forall' effect expects exactly two arguments.",
                          syntax=SYNTAX_EFFECT_FORALL)
        check_list(context, alist[1], "First argument (VARIABLES) of 'forall'", syntax=SYNTAX_EFFECT_FORALL)
        parameters = parse_typed_list(context, alist[1])
        check_list(context, alist[2], "Second argument (EFFECT) of 'forall'", syntax=SYNTAX_EFFECT_FORALL)
        effect = parse_effect(context, alist[2], type_dict, predicate_dict,
                              term_names | {p.name for p in parameters})
        return pddl.UniversalEffect(parameters, effect)
    elif tag == "when":
        if len(alist) != 3:
            context.error("'when' effect expects exactly two arguments.",
                          syntax=SYNTAX_EFFECT_WHEN)
        check_list(context, alist[1], "First argument (CONDITION) of 'when'", syntax=SYNTAX_EFFECT_WHEN)
        condition = parse_condition(context, alist[1], type_dict, predicate_dict, term_names)
        check_list(context, alist[2], "Second argument (EFFECT) of 'when'", syntax=SYNTAX_EFFECT_WHEN)
        effect = parse_effect(context, alist[2], type_dict, predicate_dict, term_names)
        return pddl.ConditionalEffect(condition, effect)
    elif tag == "increase":
        if len(alist) != 3 or alist[1] != ["total-cost"]:
            context.error("'increase' expects two arguments",
                          alist, syntax=SYNTAX_EFFECT_INCREASE)
        assignment = parse_assignment(context, alist)
        return pddl.CostEffect(assignment)
    else:
        # We pass in {} instead of type_dict here because types must
        # be static predicates, so cannot be the target of an effect.
        return pddl.SimpleEffect(parse_literal(context, alist, {}, predicate_dict, term_names))


def parse_expression(context, exp):
    with context.layer("Parsing expression"):
        if isinstance(exp, list):
            if len(exp) < 1:
                context.error("Expression cannot be an empty block.",
                              syntax=SYNTAX_EXPRESSION)
            functionsymbol = exp[0]
            return pddl.PrimitiveNumericExpression(functionsymbol, exp[1:])
        elif exp[0] == "-":
            context.error("Negative numbers are not allowed.", exp,
                          syntax=SYNTAX_EXPRESSION)
        elif exp.isdigit():
            return pddl.NumericConstant(int(exp))
        elif exp.replace(".", "").isdigit():
            context.error("Fractional numbers are not supported.", exp, syntax=SYNTAX_EXPRESSION)
        else:
            return pddl.PrimitiveNumericExpression(exp, [])


def parse_assignment(context, alist):
    with context.layer("Parsing Assignment"):
        if len(alist) != 3:
            context.error("Assignment expects two arguments",
                          syntax=SYNTAX_ASSIGNMENT)
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


def parse_action(context, alist, type_dict, predicate_dict, constant_names):
    with context.layer("Parsing action name"):
        if len(alist) < 4:
            context.error("Expecting block with at least 3 arguments for an action.",
                          syntax=SYNTAX_ACTION)
        iterator = iter(alist)
        action_tag = next(iterator)
        assert action_tag == ":action"
        name = next(iterator)
        check_word(context, name, "Action name", syntax=SYNTAX_ACTION)
    with context.layer(f"Parsing action '{name}'"):
        try:
            with context.layer("Parsing parameters"):
                parameters_tag_opt = next(iterator)
                if parameters_tag_opt == ":parameters":
                    parameters_list = next(iterator)
                    check_list(context, parameters_list, "Parameters", syntax=SYNTAX_ACTION)
                    parameters = parse_typed_list(
                        context, parameters_list, only_variables=True)
                    precondition_tag_opt = next(iterator)
                else:
                    parameters = []
                    precondition_tag_opt = parameters_tag_opt
            term_names = constant_names | {p.name for p in parameters}
            with context.layer("Parsing precondition"):
                if precondition_tag_opt == ":precondition":
                    precondition_list = next(iterator)
                    check_list(context, precondition_list, "Precondition", syntax=SYNTAX_ACTION)
                    precondition = parse_condition(
                        context, precondition_list, type_dict, predicate_dict, term_names)
                    effect_tag = next(iterator)
                else:
                    precondition = pddl.Conjunction([])
                    effect_tag = precondition_tag_opt
            with context.layer("Parsing effect"):
                if effect_tag != ":effect":
                    context.error(
                        "Effect tag is expected to be ':effect'", effect_tag,
                        syntax=SYNTAX_ACTION)
                effect_list = next(iterator)
                check_list(context, effect_list, "Effect", syntax=SYNTAX_ACTION)
                eff = []
                if effect_list:
                    cost = parse_effects(
                        context, effect_list, eff, type_dict, predicate_dict, term_names)
        except StopIteration:
            context.error(f"Missing fields. Expecting {SYNTAX_ACTION}.")
        for _ in iterator:
            context.error(f"Too many fields. Expecting {SYNTAX_ACTION}")
    if eff:
        return pddl.Action(name, parameters, len(parameters),
                           precondition, eff, cost)
    else:
        return None


def parse_axiom(context, alist, type_dict, predicate_dict, constant_names):
    with context.layer("Parsing derived predicate"):
        if len(alist) != 3:
            context.error("Expecting block with exactly three elements",
                          syntax=SYNTAX_AXIOM)
        assert alist[0] == ":derived"
        check_list(context, alist[1], "The first argument (PREDICATE)", syntax=SYNTAX_AXIOM)
        predicate = parse_predicate(context, alist[1])
    with context.layer(f"Parsing condition for derived predicate '{predicate}'"):
        if not isinstance(alist[2], list):
            context.error("The second argument (CONDITION) is expected to be a block.",
                          syntax=SYNTAX_AXIOM)
        term_names = constant_names | {a.name for a in predicate.arguments}
        condition = parse_condition(
            context, alist[2], type_dict, predicate_dict, term_names)
        return pddl.Axiom(predicate.name, predicate.arguments,
                          len(predicate.arguments), condition)


def parse_axioms_and_actions(context, entries, type_dict, predicate_dict, constant_names):
    the_axioms = []
    the_actions = []
    for no, entry in enumerate(entries, start=1):
        with context.layer(f"Parsing axiom/action entry #{no}"):
            check_named_block(context, entry, [":derived", ":action"])
            if entry[0] == ":derived":
                with context.layer(f"Parsing {len(the_axioms) + 1}. axiom"):
                    the_axioms.append(parse_axiom(
                        context, entry, type_dict, predicate_dict, constant_names))
            else:
                assert entry[0] == ":action"
                with context.layer(f"Parsing action #{len(the_actions) + 1}"):
                    action = parse_action(
                        context, entry, type_dict, predicate_dict, constant_names)
                    if action is not None:
                        the_actions.append(action)
    return the_axioms, the_actions


def parse_init(context, alist, predicate_dict, term_names):
    initial = []
    initial_proposition_values = dict()
    initial_assignments = dict()
    for no, fact in enumerate(alist[1:], start=1):
        with context.layer(f"Parsing element #{no} in init block"):
            if not isinstance(fact, list) or not fact:
                context.error(
                    "Invalid fact.",
                    syntax=f"{SYNTAX_LITERAL_POSSIBLY_NEGATED} or {SYNTAX_ASSIGNMENT}")
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
                        print_warning(f"{assignment} is specified twice "
                                      f"in initial state specification")
                    else:
                        context.error("Error in initial state specification\n"
                                      "Reason: conflicting assignment for "
                                      f"{assignment.fluent}.")
                else:
                    initial_assignments[assignment.fluent] = assignment
                    initial.append(assignment)
            else:
                atom_value = False if fact[0] == "not" else True
                if atom_value is False:
                    if len(fact) != 2:
                        context.error(f"Expecting {SYNTAX_LITERAL_NEGATED} for negated atoms.")
                    fact = fact[1]
                    if not isinstance(fact, list) or not fact:
                        context.error("Invalid negated fact.", syntax=SYNTAX_LITERAL_NEGATED)
                check_predicate_and_terms_existence(
                    context, fact[0], fact[1:],
                    predicate_dict.keys(), term_names)
                expected_predicate_arity = len(predicate_dict[fact[0]].arguments)
                predicate_arity = len(fact[1:])
                if expected_predicate_arity != predicate_arity:
                    context.error(f"Predicate '{fact[0]}' of arity {expected_predicate_arity} used"
                              f" with {predicate_arity} arguments.", fact)
                atom = pddl.Atom(fact[0], fact[1:])
                check_atom_consistency(context, atom,
                                       initial_proposition_values, atom_value)
                initial_proposition_values[atom] = atom_value
    initial.extend(atom for atom, val in initial_proposition_values.items()
                   if val is True)
    return initial


def parse_task(domain_pddl, task_pddl):
    context = Context()
    if not isinstance(domain_pddl, list):
        context.error("Invalid definition of a PDDL domain.")
    domain_name, domain_requirements, types, type_dict, constants, predicates, \
        predicate_dict, functions, actions, axioms = parse_domain_pddl(context, domain_pddl)
    if not isinstance(task_pddl, list):
        context.error("Invalid definition of a PDDL task.")
    task_name, task_domain_name, task_requirements, objects, init, goal, \
        use_metric = parse_task_pddl(context, task_pddl, type_dict,
                                     predicate_dict, {c.name for c in constants})

    if domain_name != task_domain_name:
        context.error(f"The domain name specified by the task "
                      f"({task_domain_name}) does not match the name specified "
                      f"by the domain file ({domain_name}).")
    requirements = pddl.Requirements(sorted(set(
        domain_requirements.requirements +
        task_requirements.requirements)))
    objects = constants + objects

    check_for_duplicates(context, [o.name for o in objects], "object")
    check_for_duplicates(context, [a.name for a in actions], "action")

    init += [pddl.Atom("=", (obj.name, obj.name)) for obj in objects]

    return pddl.Task(
        domain_name, task_name, requirements, types, objects,
        predicates, functions, init, goal, actions, axioms, use_metric)


def parse_domain_pddl(context, domain_pddl):
    if len(domain_pddl) < 2:
        context.error("The domain file must start with the define keyword and the domain name.")
    iterator = iter(domain_pddl)
    with context.layer("Parsing domain"):
        define_tag = next(iterator)
        if define_tag != "define":
            context.error(f"Domain definition expected to start with '(define '. Got '({define_tag}'")

        with context.layer("Parsing domain name"):
            domain_line = next(iterator)
            check_named_block(context, domain_line, ["domain"], syntax=SYNTAX_DOMAIN_DOMAIN_NAME)
            if len(domain_line) != 2 or not isinstance(domain_line[1], str):
                context.error("The definition of the domain name expects exactly one word after 'domain'.",
                              domain_line[1:], syntax=SYNTAX_DOMAIN_DOMAIN_NAME)
            yield domain_line[1]

        ## We allow an arbitrary order of the requirement, types, constants,
        ## predicates and functions specification. The PDDL BNF is more strict on
        ## this, so we print a warning if it is violated.
        requirements = pddl.Requirements([":strips"])
        the_types = [pddl.Type("object")]
        constants, the_predicates, the_functions = [], [], []
        correct_order = [":requirements", ":types", ":constants", ":predicates",
                         ":functions"]
        action_or_axiom_block = [":derived", ":action"]
        seen_fields = []
        first_action = None
        for opt in iterator:
            check_named_block(context, opt, correct_order + action_or_axiom_block)
            field = opt[0]
            if field not in correct_order:
                first_action = opt
                break
            if field in seen_fields:
                context.error(f"Error in domain specification\n"
                              f"Reason: two '{field}' specifications.")
            if (seen_fields and
                    correct_order.index(seen_fields[-1]) > correct_order.index(field)):
                msg = f"{field} specification not allowed here (cf. PDDL BNF)"
                print_warning(msg)
            seen_fields.append(field)
            if field == ":requirements":
                requirements = parse_requirements(context, opt[1:])
            elif field == ":types":
                with context.layer("Parsing types"):
                    the_types.extend(parse_typed_list(
                        context, opt[1:], constructor=construct_type))
                    for t in the_types:
                        if t.name == "number":
                            context.error('Encountered declaration of type "number", which is a reserved type that cannot be redeclared.')
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
            context, entries, type_dict, predicate_dict, {c.name for c in constants})

        yield the_actions
        yield the_axioms


def parse_task_pddl(context, task_pddl, type_dict, predicate_dict, constant_names):
    iterator = iter(task_pddl)
    with context.layer("Parsing task"):
        try:
            define_tag = next(iterator)
            if define_tag != "define":
                context.error("Task definition expected to start with '(define ")

            with context.layer("Parsing problem name"):
                problem_line = next(iterator)
                check_named_block(context, problem_line, ["problem"], syntax=SYNTAX_TASK_PROBLEM_NAME)
                if len(problem_line) != 2 or not isinstance(problem_line[1], str):
                    context.error("The definition of the problem name expects exactly one word after 'problem'.",
                                  problem_line[1:], syntax=SYNTAX_TASK_PROBLEM_NAME)
                yield problem_line[1]

            with context.layer("Parsing domain name"):
                domain_line = next(iterator)
                check_named_block(context, domain_line, [":domain"], syntax=SYNTAX_TASK_DOMAIN_NAME)
                if len(domain_line) != 2 or not isinstance(domain_line[1], str):
                    context.error("The definition of the domain name expects exactly one word after ':domain'.",
                                  domain_line[1:], syntax=SYNTAX_TASK_DOMAIN_NAME)
                yield domain_line[1]

            requirements_opt = next(iterator)
            check_named_block(context, requirements_opt, [":requirements", ":objects", ":init"])
            if requirements_opt[0] == ":requirements":
                requirements = requirements_opt[1:]
                objects_opt = next(iterator)
            else:
                requirements = []
                objects_opt = requirements_opt
            yield parse_requirements(context, requirements)

            check_named_block(context, objects_opt, [":objects", ":init"])
            objects = []
            if objects_opt[0] == ":objects":
                with context.layer("Parsing objects"):
                    objects = parse_typed_list(context, objects_opt[1:])
                    yield objects
                init = next(iterator)
            else:
                yield []
                init = objects_opt

            check_named_block(context, init, [":init"])
            term_names = constant_names | {o.name for o in objects}
            yield parse_init(context, init, predicate_dict, term_names)

            goal = next(iterator)
            with context.layer("Parsing goal"):
                check_named_block(context, goal, [":goal"], syntax=SYNTAX_GOAL)
                if len(goal) != 2 or not isinstance(goal[1], list) or not goal[1]:
                    context.error("The definition of the goal expects a non-empty list after ':goal'.",
                                  goal[1:], syntax=SYNTAX_GOAL)
                yield parse_condition(context, goal[1], type_dict, predicate_dict, term_names)
        except StopIteration:
            context.error(
                "The problem file must contain at least the following five fields: define-keyword, problem name, domain name, initial state, and goal.")

        use_metric = False
        try:
            metric = next(iterator)
        except StopIteration:
            # nothing more to parse
            yield use_metric
            return
        if not isinstance(metric, list) or not metric or metric[0] != ":metric":
            context.error("After the goal nothing is allowed except the definition of the total-cost metric.", metric,
                          syntax=SYNTAX_METRIC)
        with context.layer("Parsing metric"):
            if len(metric) != 3 or not isinstance(metric[2], list) or len(metric[2]) != 1 or metric[1] != "minimize" or \
                    metric[2][0] != "total-cost":
                context.error("Invalid metric definition.", metric, syntax=SYNTAX_METRIC)
            use_metric = True
        yield use_metric

        for entry in iterator:
            previous_entry = "metric" if use_metric else "goal"
            context.error(f"After the {previous_entry} nothing is allowed.", entry)


def check_atom_consistency(context, atom, initial_proposition_values,
                           atom_value):
    if atom in initial_proposition_values:
        prev_value = initial_proposition_values[atom]
        if prev_value != atom_value:
            context.error(f"Error in initial state specification\n"
                          f"Reason: {atom} is true and false.")
        else:
            if atom_value is False:
                atom = atom.negate()
            print_warning(f"{atom} is specified twice in initial state specification")


def check_predicate_and_terms_existence(
        context, predicate_name, terms, valid_predicate_names, valid_term_names):
    assert isinstance(valid_predicate_names, type({}.keys()))
    assert isinstance(valid_term_names, set)
    if predicate_name not in valid_predicate_names:
        context.error("Undefined predicate", predicate_name)
    for term in terms:
        if term not in valid_term_names:
            item = "variable" if term.startswith("?") else "object"
            context.error(f"Undefined {item}", term)


def check_for_duplicates(context, elements, element_type):
    seen = set()
    duplicates = set()
    for element in elements:
        if element in seen:
            duplicates.add(element)
        else:
            seen.add(element)
    if duplicates:
        msg = f"Found the following duplicate {element_type}s: {', '.join(duplicates)}"
        if element_type == "action":
            print_warning(msg)
        else:
            context.error(msg)
