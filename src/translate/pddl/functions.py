from . import pddl_types

class Function(object):
    def __init__(self, name, arguments, type):
        self.name = name
        self.arguments = arguments
        if (type != "number"):
            raise SystemExit("Error: object fluents not supported\n" +
                             "(function %s has type %s)" % (name, type))
        self.type = type
    @classmethod
    def parse(cls, alist, type):
        name = alist[0]
        arguments = pddl_types.parse_typed_list(alist[1:])
        return cls(name, arguments, type)
    def __str__(self):
        result = "%s(%s)" % (self.name, ", ".join(map(str, self.arguments)))
        if self.type:
            result += ": %s" % self.type
        return result
