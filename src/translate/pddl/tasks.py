from __future__ import print_function

import sys

from . import actions
from . import axioms
from . import conditions
from . import predicates
from . import pddl_types
from . import functions
from . import f_expression

class Task(object):
    def __init__(self, domain_name, task_name, requirements,
                 types, objects, predicates, functions, init, goal, actions, axioms, use_metric):
        self.domain_name = domain_name
        self.task_name = task_name
        self.requirements = requirements
        self.types = types
        self.objects = objects
        self.predicates = predicates
        self.functions = functions
        self.init = init
        self.goal = goal
        self.actions = actions
        self.axioms = axioms
        self.axiom_counter = 0
        self.use_min_cost_metric = use_metric

    def add_axiom(self, parameters, condition):
        name = "new-axiom@%d" % self.axiom_counter
        self.axiom_counter += 1
        axiom = axioms.Axiom(name, parameters, len(parameters), condition)
        self.predicates.append(predicates.Predicate(name, parameters))
        self.axioms.append(axiom)
        return axiom

    @staticmethod
    def parse(domain_pddl, task_pddl):
        domain_name, domain_requirements, types, constants, predicates, functions, actions, axioms \
                     = parse_domain(domain_pddl)
        task_name, task_domain_name, task_requirements, objects, init, goal, use_metric = parse_task(task_pddl)

        assert domain_name == task_domain_name
        requirements = Requirements(sorted(set(
                    domain_requirements.requirements +
                    task_requirements.requirements)))
        objects = constants + objects
        check_for_duplicates(
            [o.name for o in objects],
            errmsg="error: duplicate object %r",
            finalmsg="please check :constants and :objects definitions")
        init += [conditions.Atom("=", (obj.name, obj.name)) for obj in objects]

        return Task(domain_name, task_name, requirements, types, objects,
                    predicates, functions, init, goal, actions, axioms, use_metric)

    def dump(self):
        print("Problem %s: %s [%s]" % (
            self.domain_name, self.task_name, self.requirements))
        print("Types:")
        for type in self.types:
            print("  %s" % type)
        print("Objects:")
        for obj in self.objects:
            print("  %s" % obj)
        print("Predicates:")
        for pred in self.predicates:
            print("  %s" % pred)
        print("Functions:")
        for func in self.functions:
            print("  %s" % func)
        print("Init:")
        for fact in self.init:
            print("  %s" % fact)
        print("Goal:")
        self.goal.dump()
        print("Actions:")
        for action in self.actions:
            action.dump()
        if self.axioms:
            print("Axioms:")
            for axiom in self.axioms:
                axiom.dump()

class Requirements(object):
    def __init__(self, requirements):
        self.requirements = requirements
        for req in requirements:
            assert req in (
              ":strips", ":adl", ":typing", ":negation", ":equality",
              ":negative-preconditions", ":disjunctive-preconditions",
              ":existential-preconditions", ":universal-preconditions",
              ":quantified-preconditions", ":conditional-effects",
              ":derived-predicates", ":action-costs"), req
    def __str__(self):
        return ", ".join(self.requirements)

def parse_domain(domain_pddl):
    iterator = iter(domain_pddl)

    define_tag = next(iterator)
    assert define_tag == "define"
    domain_line = next(iterator)
    assert domain_line[0] == "domain" and len(domain_line) == 2
    yield domain_line[1]

    ## We allow an arbitrary order of the requirement, types, constants,
    ## predicates and functions specification. The PDDL BNF is more strict on
    ## this, so we print a warning if it is violated.
    requirements = Requirements([":strips"])
    the_types = [pddl_types.Type("object")]
    constants, the_predicates, the_functions = [], [], []
    correct_order = [":requirements", ":types", ":constants", ":predicates",
                     ":functions"]
    seen_fields = []
    for opt in iterator:
        field = opt[0]
        if field not in correct_order:
            first_action = opt
            break
        if field in seen_fields:
            raise SystemExit("Error in domain specification\n" +
                             "Reason: two '%s' specifications." % field)
        if (seen_fields and
            correct_order.index(seen_fields[-1]) > correct_order.index(field)):
            msg = "\nWarning: %s specification not allowed here (cf. PDDL BNF)" % field
            print(msg, file=sys.stderr)
        seen_fields.append(field)
        if field == ":requirements":
            requirements = Requirements(opt[1:])
        elif field == ":types":
            the_types.extend(pddl_types.parse_typed_list(opt[1:],
                        constructor=pddl_types.Type))
        elif field == ":constants":
            constants = pddl_types.parse_typed_list(opt[1:])
        elif field == ":predicates":
            the_predicates = [predicates.Predicate.parse(entry)
                              for entry in opt[1:]]
            the_predicates += [predicates.Predicate("=",
                                 [pddl_types.TypedObject("?x", "object"),
                                  pddl_types.TypedObject("?y", "object")])]
        elif field == ":functions":
            the_functions = pddl_types.parse_typed_list(
                opt[1:],
                constructor=functions.Function.parse,
                default_type="number")
    pddl_types.set_supertypes(the_types)
    # for type in the_types:
    #   print repr(type), type.supertype_names
    yield requirements
    yield the_types
    yield constants
    yield the_predicates
    yield the_functions

    entries = [first_action] + [entry for entry in iterator]
    the_axioms = []
    the_actions = []
    for entry in entries:
        if entry[0] == ":derived":
            axiom = axioms.Axiom.parse(entry)
            the_axioms.append(axiom)
        else:
            action = actions.Action.parse(entry)
            the_actions.append(action)
    yield the_actions
    yield the_axioms

def parse_task(task_pddl):
    iterator = iter(task_pddl)

    define_tag = next(iterator)
    assert define_tag == "define"
    problem_line = next(iterator)
    assert problem_line[0] == "problem" and len(problem_line) == 2
    yield problem_line[1]
    domain_line = next(iterator)
    assert domain_line[0] == ":domain" and len(domain_line) == 2
    yield domain_line[1]

    requirements_opt = next(iterator)
    if requirements_opt[0] == ":requirements":
        requirements = requirements_opt[1:]
        objects_opt = next(iterator)
    else:
        requirements = []
        objects_opt = requirements_opt
    yield Requirements(requirements)

    if objects_opt[0] == ":objects":
        yield pddl_types.parse_typed_list(objects_opt[1:])
        init = next(iterator)
    else:
        yield []
        init = objects_opt

    assert init[0] == ":init"
    initial = []
    initial_true = set()
    initial_false = set()
    initial_assignments = dict()
    for fact in init[1:]:
        if fact[0] == "=":
            try:
                assignment = f_expression.parse_assignment(fact)
            except ValueError as e:
                raise SystemExit("Error in initial state specification\n" +
                                 "Reason: %s." %  e)
            if not isinstance(assignment.expression,
                              f_expression.NumericConstant):
                raise SystemExit("Illegal assignment in initial state " +
                    "specification:\n%s" % assignment)
            if assignment.fluent in initial_assignments:
                prev = initial_assignments[assignment.fluent]
                if assignment.expression == prev.expression:
                    print("Warning: %s is specified twice" % assignment,
                          "in initial state specification")
                else:
                    raise SystemExit("Error in initial state specification\n" +
                                     "Reason: conflicting assignment for " +
                                     "%s." %  assignment.fluent)
            else:
                initial_assignments[assignment.fluent] = assignment
                initial.append(assignment)
        elif fact[0] == "not":
            atom = conditions.Atom(fact[1][0], fact[1][1:])
            check_atom_consistency(atom, initial_false, initial_true, False)
            initial_false.add(atom)
        else:
            atom = conditions.Atom(fact[0], fact[1:])
            check_atom_consistency(atom, initial_true, initial_false)
            initial_true.add(atom)
    initial.extend(initial_true)
    yield initial

    goal = next(iterator)
    assert goal[0] == ":goal" and len(goal) == 2
    yield conditions.parse_condition(goal[1])

    use_metric = False
    for entry in iterator:
        if entry[0] == ":metric":
            if entry[1]=="minimize" and entry[2][0] == "total-cost":
                use_metric = True
            else:
                assert False, "Unknown metric."
    yield use_metric

    for entry in iterator:
        assert False, entry

def check_atom_consistency(atom, same_truth_value, other_truth_value, atom_is_true=True):
    if atom in other_truth_value:
        raise SystemExit("Error in initial state specification\n" +
                         "Reason: %s is true and false." %  atom)
    if atom in same_truth_value:
        if not atom_is_true:
            atom = atom.negate()
        print("Warning: %s is specified twice in initial state specification" % atom)
    

def check_for_duplicates(elements, errmsg, finalmsg):
    seen = set()
    errors = []
    for element in elements:
        if element in seen:
            errors.append(errmsg % element)
        else:
            seen.add(element)
    if errors:
        raise SystemExit("\n".join(errors) + "\n" + finalmsg)

