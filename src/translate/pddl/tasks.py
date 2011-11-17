import sys

import actions
import axioms
import conditions
import predicates
import pddl_types
import functions
import f_expression

class Task(object):
    FUNCTION_SYMBOLS = dict()
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
        domain_name, requirements, types, constants, predicates, functions, actions, axioms \
                     = parse_domain(domain_pddl)
        task_name, task_domain_name, objects, init, goal, use_metric = parse_task(task_pddl)

        assert domain_name == task_domain_name
        objects = constants + objects
        init += [conditions.Atom("=", (obj.name, obj.name)) for obj in objects]

        return Task(domain_name, task_name, requirements, types, objects,
                    predicates, functions, init, goal, actions, axioms, use_metric)

    def dump(self):
        print "Problem %s: %s [%s]" % (self.domain_name, self.task_name,
                                       self.requirements)
        print "Types:"
        for type in self.types:
            print "  %s" % type
        print "Objects:"
        for obj in self.objects:
            print "  %s" % obj
        print "Predicates:"
        for pred in self.predicates:
            print "  %s" % pred
        print "Functions:"
        for func in self.functions:
            print "  %s" % func
        print "Init:"
        for fact in self.init:
            print "  %s" % fact
        print "Goal:"
        self.goal.dump()
        print "Actions:"
        for action in self.actions:
            action.dump()
        if self.axioms:
            print "Axioms:"
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

    assert iterator.next() == "define"
    domain_line = iterator.next()
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
            print >> sys.stderr, msg
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
            the_functions = pddl_types.parse_typed_list(opt[1:],
                    constructor=functions.Function.parse_typed, functions=True)
            for function in the_functions:
                Task.FUNCTION_SYMBOLS[function.name] = function.type
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

    assert iterator.next() == "define"
    problem_line = iterator.next()
    assert problem_line[0] == "problem" and len(problem_line) == 2
    yield problem_line[1]
    domain_line = iterator.next()
    assert domain_line[0] == ":domain" and len(domain_line) == 2
    yield domain_line[1]

    objects_opt = iterator.next()
    if objects_opt[0] == ":objects":
        yield pddl_types.parse_typed_list(objects_opt[1:])
        init = iterator.next()
    else:
        yield []
        init = objects_opt

    assert init[0] == ":init"
    initial = []
    for fact in init[1:]:
        if fact[0] == "=":
            try:
                initial.append(f_expression.parse_assignment(fact))
            except ValueError, e:
                raise SystemExit("Error in initial state specification\n" +
                                 "Reason: %s." %  e)
        else:
            initial.append(conditions.Atom(fact[0], fact[1:]))
    yield initial

    goal = iterator.next()
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
