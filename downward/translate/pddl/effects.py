import conditions
import pddl_types

def cartesian_product(*sequences):
  # TODO: Also exists in tools.py outside the pddl package (defined slightly
  #       differently). Not good. Need proper import paths.
  if not sequences:
    yield ()
  else:
    for tup in cartesian_product(*sequences[1:]):
      for item in sequences[0]:
        yield (item,) + tup

def parse_effects(alist, result):
  """Parse a PDDL effect (simple, negative, conditional, universal conditional).
  Only understands effects with a restricted syntax:
    <effect> ::= {forall <parameters>}* <conditional-effect>
    <conditional-effect> ::= {when <condition>}? <simple-effect>
    <simple-effect> ::= {not}? <fact>"""

  orig_alist = alist # HACK!

  parameters = []
  while alist[0] == "forall":
    assert len(alist) == 3
    parameters += pddl_types.parse_typed_list(alist[1])
    alist = alist[2]

  if alist[0] == "when":
    assert len(alist) == 3
    condition = conditions.parse_condition(alist[1])
    alist = alist[2]
  else:
    condition = conditions.Truth()

  if alist[0] == "and": # This branch is a HACK!
    if len(alist) > 1:
      literal = conditions.parse_literal(alist[-1])
      effect = Effect(parameters, condition, literal)
      result.append(effect)
      alist[:] = alist[:-1]
      parse_effects(orig_alist, result)
  else:
    literal = conditions.parse_literal(alist)
    effect = Effect(parameters, condition, literal)
    result.append(effect)
  
class Effect(object):
  def __init__(self, parameters, condition, literal):
    self.parameters = parameters
    self.condition = condition
    self.literal = literal
  def dump(self):
    indent = "  "
    if self.parameters:
      print "%sforall %s" % (indent, ", ".join(map(str, self.parameters)))
      indent += "  "
    if self.condition != conditions.Truth():
      print "%sif" % indent
      self.condition.dump(indent + "  ")
      print "%sthen" % indent
      indent += "  "
    print "%s%s" % (indent, self.literal)
  def uniquify_variables(self, type_map):
    renamings = {}
    self.parameters = [par.uniquify_name(type_map, renamings)
                       for par in self.parameters]
    self.condition = self.condition.uniquify_variables(type_map, renamings)
    self.literal = self.literal.rename_variables(renamings)
#  def build_prolog_rules(self, prog, rule_body):
#    assert not self.literal.negated, "Something fishy"
#    assert self.condition == conditions.Truth(), "Not implemented"
#    prog.add_rule(rule_body, self.literal)
#  def to_untyped_strips(self):
#    # Used by pddl_to_prolog.py for generating Horn rules.
#    # TODO: This is somewhat hackish and incomplete for ADL.
#    #       Should be replaced by something more reasonable.
#    assert not self.literal.negated, "Something strange here"
#    conditions = [par.to_untyped_strips() for par in self.parameters]
#    conditions += self.condition.to_untyped_strips()
#    return conditions, self.literal
  def instantiate(self, var_mapping, init_facts, fluent_facts,
                  objects_by_type, result):
    if self.parameters:
      var_mapping = var_mapping.copy() # Will modify this.
      object_lists = [objects_by_type.get(par.type, [])
                      for par in self.parameters]
      for object_tuple in cartesian_product(*object_lists):
        for (par, obj) in zip(self.parameters, object_tuple):
          var_mapping[par.name] = obj
        self._instantiate(var_mapping, init_facts, fluent_facts, result)
    else:
      self._instantiate(var_mapping, init_facts, fluent_facts, result)
  def _instantiate(self, var_mapping, init_facts, fluent_facts, result):
    condition = []
    try:
      self.condition.instantiate(var_mapping, init_facts, fluent_facts, condition)
    except conditions.Impossible:
      return
    effects = []
    self.literal.instantiate(var_mapping, init_facts, fluent_facts, effects)
    assert len(effects) <= 1
    if effects:
      result.append((condition, effects[0]))
  def relaxed(self):
    if self.literal.negated:
      return None
    else:
      return Effect(self.parameters, self.condition.relaxed(), self.literal)
  def simplified(self):
    return Effect(self.parameters, self.condition.simplified(), self.literal)
