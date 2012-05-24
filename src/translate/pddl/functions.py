from . import pddl_types

class Function(object):
    def __init__(self, name, arguments):
        self.name = name
        self.arguments = arguments
    @classmethod
    def parse(cls, alist):
        name = alist[0]
        arguments = pddl_types.parse_typed_list(alist[1:],
                                                default_type="number")
        return cls(name, arguments)
    def __str__(self):
        result = "%s(%s)" % (self.name, ", ".join(map(str, self.arguments)))
        if self.type:
            result += ": %s" % self.type
        return result
