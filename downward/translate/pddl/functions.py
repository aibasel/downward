import pddl_types

class Function(object):
    def __init__(self, name, arguments):
        self.name = name
        self.arguments = arguments
    def parse(alist):
        name = alist[0]
        arguments = pddl_types.parse_typed_list(alist[1:], functions=True)
        return Function(name, arguments)
    def parse_typed(alist, _type):
        function = Function.parse(alist)
        function.type = _type
        return function
    parse = staticmethod(parse)
    parse_typed = staticmethod(parse_typed)
    def __str__(self):
        if self.type:
            return "%s(%s): %s" % (self.name, ", ".join(map(str, self.arguments)), self.type) 
        else:
            return "%s(%s)" % (self.name, ", ".join(map(str, self.arguments))) 

