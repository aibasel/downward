from . import pddl_types


class Predicate(object):
    def __init__(self, name, arguments):
        self.name = name
        self.arguments = arguments

    @classmethod
    def parse(cls, alist):
        name = alist[0]
        arguments = pddl_types.parse_typed_list(alist[1:], only_variables=True)
        return cls(name, arguments)

    def __str__(self):
        return "%s(%s)" % (self.name, ", ".join(map(str, self.arguments)))

    def get_arity(self):
        return len(self.arguments)
