# Renamed from types.py to avoid clash with stdlib module.
# In the future, use explicitly relative imports or absolute
# imports as a better solution.

import graph

import itertools

class Type(object):
    def __init__(self, name, basetype_name=None):
        self.name = name
        self.basetype_name = basetype_name
    def __str__(self):
        return self.name
    def __repr__(self):
        return "Type(%s, %s)" % (self.name, self.basetype_name)

def set_supertypes(type_list):
    typename_to_type = {}
    child_types = []
    for type in type_list:
        type.supertype_names = []
        typename_to_type[type.name] = type
        if type.basetype_name:
            child_types.append((type.name, type.basetype_name))
    for (desc_name, anc_name) in graph.transitive_closure(child_types):
        typename_to_type[desc_name].supertype_names.append(anc_name)


class TypedObject(object):
    def __init__(self, name, type):
        self.name = name
        self.type = type
    def __hash__(self):
        return hash((self.name, self.type))
    def __eq__(self, other):
        return self.name == other.name and self.type == other.type
    def __ne__(self, other):
        return not self == other
    def __str__(self):
        return "%s: %s" % (self.name, self.type)
    def __repr__(self):
        return "<TypedObject %s: %s>" % (self.name, self.type)
    def uniquify_name(self, type_map, renamings):
        if self.name not in type_map:
            type_map[self.name] = self.type
            return self
        for counter in itertools.count(1):
            new_name = self.name + str(counter)
            if new_name not in type_map:
                renamings[self.name] = new_name
                type_map[new_name] = self.type
                return TypedObject(new_name, self.type)
    def to_untyped_strips(self):
        # TODO: Try to resolve the cyclic import differently.
        # Avoid cyclic import.
        from . import conditions
        return conditions.Atom(self.type, [self.name])


def parse_typed_list(alist, only_variables=False, constructor=TypedObject,
                     default_type="object"):
    result = []
    while alist:
        try:
            separator_position = alist.index("-")
        except ValueError:
            items = alist
            _type = default_type
            alist = []
        else:
            items = alist[:separator_position]
            _type = alist[separator_position + 1]
            alist = alist[separator_position + 2:]
        for item in items:
            assert not only_variables or item.startswith("?"), \
                   "Expected item to be a variable: %s in (%s)" % (
                item, " ".join(items))
            entry = constructor(item, _type)
            result.append(entry)
    return result
