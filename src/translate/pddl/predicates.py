from .pddl_types import TypedObject

class Predicate:
    def __init__(self, name: str, arguments: list[TypedObject]) -> None:
        self.name = name
        self.arguments = arguments

    def __str__(self):
        return "%s(%s)" % (self.name, ", ".join(map(str, self.arguments)))

    def get_arity(self):
        return len(self.arguments)
