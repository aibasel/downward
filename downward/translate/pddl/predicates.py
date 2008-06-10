import pddl_types

class Predicate(object):
    def __init__(self, name, arguments):
        self.name = name
        self.arguments = arguments
    def parse(alist):
        name = alist[0]
        arguments = pddl_types.parse_typed_list(alist[1:], only_variables=True)
        return Predicate(name, arguments)
    parse = staticmethod(parse)
    def __str__(self):
        return "%s(%s)" % (self.name, ", ".join(map(str, self.arguments)))

