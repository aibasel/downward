import pddl_types

class Function(object):
    def __init__(self, name, arguments):
        self.name = name
        self.arguments = arguments
    @classmethod
    def parse(cls, alist):
        name = alist[0]
        arguments = pddl_types.parse_typed_list(alist[1:], functions=True)
        return cls(name, arguments)
    @classmethod
    def parse_typed(cls, alist, _type):
        function = cls.parse(alist)
        function.type = _type
        return function
    def __str__(self):
        result = "%s(%s)" % (self.name, ", ".join(map(str, self.arguments)))
        if self.type:
            result += ": %s" % self.type
        return result
