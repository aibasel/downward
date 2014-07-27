# Renamed from types.py to avoid clash with stdlib module.
# In the future, use explicitly relative imports or absolute
# imports as a better solution.

import graph

import itertools


def get_type_id(typename):
    # PDDL allows mixing types and predicates, but some PDDL files
    # have name collisions between types and predicates. To support
    # this, we give types string IDs that cannot be confused with
    # non-type predicates.
    return "type@%s" % typename


class Type(object):
    def __init__(self, name, basetype_name=None):
        self.name = name
        self.id = get_type_id(name)
        if basetype_name is None:
            self.basetype_id = None
        else:
            self.basetype_id = get_type_id(basetype_name)

    def __str__(self):
        return self.id

    def __repr__(self):
        return "Type(%s, %s)" % (self.id, self.basetype_id)


def set_supertypes(type_list):
    typeid_to_type = {}
    child_types = []
    for type in type_list:
        type.supertype_ids = []
        typeid_to_type[type.id] = type
        if type.basetype_id:
            child_types.append((type.id, type.basetype_id))
    for (desc_id, anc_id) in graph.transitive_closure(child_types):
        typeid_to_type[desc_id].supertype_ids.append(anc_id)


class TypedObject(object):
    def __init__(self, name, type_name):
        self.name = name
        self.type_id = get_type_id(type_name)

    def __hash__(self):
        return hash((self.name, self.type_id))

    def __eq__(self, other):
        return self.name == other.name and self.type_id == other.type_id

    def __ne__(self, other):
        return not self == other

    def __str__(self):
        return "%s: %s" % (self.name, self.type_id)

    def __repr__(self):
        return "<TypedObject %s: %s>" % (self.name, self.type_id)

    def uniquify_name(self, type_map, renamings):
        if self.name not in type_map:
            type_map[self.name] = self.type_id
            return self
        for counter in itertools.count(1):
            new_name = self.name + str(counter)
            if new_name not in type_map:
                renamings[self.name] = new_name
                type_map[new_name] = self.type_id
                return TypedObject(new_name, self.type_id)

    def to_untyped_strips(self):
        # TODO: Try to resolve the cyclic import differently.
        # Avoid cyclic import.
        from . import conditions
        return conditions.Atom(self.type_id, [self.name])


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
