from __future__ import print_function

from . import conditions
from . import predicates


class Axiom(object):
    def __init__(self, name, parameters, num_external_parameters, condition):
        # For an explanation of num_external_parameters, see the
        # related Action class. Note that num_external_parameters
        # always equals the arity of the derived predicate.
        assert 0 <= num_external_parameters <= len(parameters)
        self.name = name
        self.parameters = parameters
        self.num_external_parameters = num_external_parameters
        self.condition = condition
        self.uniquify_variables()

    @classmethod
    def parse(cls, alist, type_dict, predicate_dict):
        assert len(alist) == 3
        assert alist[0] == ":derived"
        predicate = predicates.Predicate.parse(
            alist[1], type_dict, predicate_dict)
        condition = conditions.parse_condition(
            alist[2], type_dict, predicate_dict)
        return cls(predicate.name, predicate.arguments,
                   len(predicate.arguments), condition)

    def dump(self):
        args = map(str, self.parameters[:self.num_external_parameters])
        print("Axiom %s(%s)" % (self.name, ", ".join(args)))
        self.condition.dump()

    def uniquify_variables(self):
        self.type_map = dict([(par.name, par.type_id) for par in self.parameters])
        self.condition = self.condition.uniquify_variables(self.type_map)

    def instantiate(self, var_mapping, init_facts, fluent_facts):
        # The comments for Action.instantiate apply accordingly.
        arg_list = [self.name] + [var_mapping[par.name]
                    for par in self.parameters[:self.num_external_parameters]]
        name = "(%s)" % " ".join(arg_list)

        condition = []
        try:
            self.condition.instantiate(var_mapping, init_facts, fluent_facts, condition)
        except conditions.Impossible:
            return None

        effect_args = [var_mapping.get(arg.name, arg.name)
                       for arg in self.parameters[:self.num_external_parameters]]
        effect = conditions.Atom(self.name, effect_args)
        return PropositionalAxiom(name, condition, effect)


class PropositionalAxiom:
    def __init__(self, name, condition, effect):
        self.name = name
        self.condition = condition
        self.effect = effect

    def clone(self):
        return PropositionalAxiom(self.name, list(self.condition), self.effect)

    def dump(self):
        if self.effect.negated:
            print("not", end=' ')
        print(self.name)
        for fact in self.condition:
            print("PRE: %s" % fact)
        print("EFF: %s" % self.effect)

    @property
    def key(self):
        return (self.name, self.condition, self.effect)

    def __lt__(self, other):
        return self.key < other.key

    def __le__(self, other):
        return self.key <= other.key

    def __eq__(self, other):
        return self.key == other.key

    def __repr__(self):
        return '<PropositionalAxiom %s %s -> %s>' % (
            self.name, self.condition, self.effect)
