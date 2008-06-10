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
  """Parse a PDDL effect (any combination of simple, conjunctive, conditional, and universal)."""
  effect = parse_effect(alist)
  normalized = effect.normalize()
  add_effect(normalized, result)
 

def add_effect(tmp_effect, result):
  """tmp_effect has the following structure:
     [ConjunctiveEffect] [UniversalEffect] [ConditionalEffect] SimpleEffect."""
  
  if isinstance(tmp_effect, ConjunctiveEffect):
    for effect in tmp_effect.effects:
      add_effect(effect, result)
    return
  else:
    parameters = []
    condition = conditions.Truth()
    if isinstance(tmp_effect, UniversalEffect):
      parameters = tmp_effect.parameters
      if isinstance(tmp_effect.effect, ConditionalEffect):
        condition = tmp_effect.effect.condition
        assert isinstance(tmp_effect.effect.effect, SimpleEffect)
        effect = tmp_effect.effect.effect.effect
      else:
        assert isinstance(tmp_effect.effect, SimpleEffect)
        effect = tmp_effect.effect.effect
    elif isinstance(tmp_effect, ConditionalEffect):
      condition = tmp_effect.condition
      assert isinstance(tmp_effect.effect, SimpleEffect)
      effect = tmp_effect.effect.effect
    else:
      assert isinstance(tmp_effect, SimpleEffect)
      effect = tmp_effect.effect
    assert isinstance(effect, conditions.Literal)
    result.append(Effect(parameters, condition, effect))
 
def parse_effect(alist):
    tag = alist[0]
    if tag == "and":
      return ConjunctiveEffect([parse_effect(eff) for eff in alist[1:]])
    elif tag == "forall":
      assert len(alist) == 3
      parameters = pddl_types.parse_typed_list(alist[1])
      effect = parse_effect(alist[2])
      return UniversalEffect(parameters, effect)
    elif tag == "when":
      assert len(alist) == 3
      condition = conditions.parse_condition(alist[1])
      effect = parse_effect(alist[2])
      return ConditionalEffect(condition, effect)
    else:
        return SimpleEffect(conditions.parse_literal(alist))
      

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


class ConditionalEffect(object):
  def __init__(self, condition, effect):
    if isinstance(effect, ConditionalEffect):
      self.condition = conditions.Conjunction([condition, effect.condition])
      self.effect = effect.effect
    else:
      self.condition = condition
      self.effect = effect
  def dump(self, indent="  "):
    print "%sif" % (indent)
    self.condition.dump(indent + "  ")
    print "%sthen" % (indent)
    self.effect.dump(indent + "  ")
  def normalize(self):
    norm_effect = self.effect.normalize()
    if isinstance(norm_effect, ConjunctiveEffect):
      new_effects = []
      for effect in norm_effect.effects:
        assert isinstance(effect, SimpleEffect) or isinstance(effect, ConditionalEffect)
        new_effects.append(ConditionalEffect(self.condition, effect))
      return ConjunctiveEffect(new_effects)
    elif isinstance(norm_effect, UniversalEffect):
      child = norm_effect.effect
      cond_effect = ConditionalEffect(self.condition, child)
      return UniversalEffect(norm_effect.parameters, cond_effect)
    else:
      return ConditionalEffect(self.condition, norm_effect)

class UniversalEffect(object):
  def __init__(self, parameters, effect):
    if isinstance(effect, UniversalEffect):
      self.parameters = parameters + effect.parameters
      self.effect = effect.effect
    else:
      self.parameters = parameters
      self.effect = effect
  def dump(self, indent="  "):
    print "%sforall %s" % (indent, ", ".join(map(str, self.parameters)))
    self.effect.dump(indent + "  ")
  def normalize(self):
    norm_effect = self.effect.normalize()
    if isinstance(norm_effect, ConjunctiveEffect):
      new_effects = []
      for effect in norm_effect.effects:
        assert isinstance(effect, SimpleEffect) or isinstance(effect, ConditionalEffect)\
               or isinstance(effect, UniversalEffect)
        new_effects.append(UniversalEffect(self.parameters, effect))
      return ConjunctiveEffect(new_effects)
    else:
      return UniversalEffect(self.parameters, norm_effect) 

class ConjunctiveEffect(object):
  def __init__(self, effects):
    flattened_effects = []
    for effect in effects:
      if isinstance(effect, ConjunctiveEffect):
        flattened_effects += effect.effects
      else:
        flattened_effects.append(effect)
    self.effects = flattened_effects
  def dump(self, indent="  "):
    print "%sand" % (indent)
    for eff in self.effects:
      eff.dump(indent + "  ")
  def normalize(self):
    new_effects = []
    for effect in self.effects:
      new_effects.append(effect.normalize())
    return ConjunctiveEffect(new_effects)
      
class SimpleEffect(object):
  def __init__(self, effect):
    self.effect = effect
  def dump(self, indent="  "):
    print "%s%s" % (indent, self.effect)
  def normalize(self):
    return self
