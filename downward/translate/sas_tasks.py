class SASTask:
  def __init__(self, variables, init, goal, operators, axioms):
    self.variables = variables
    self.init = init
    self.goal = goal
    self.operators = operators
    self.axioms = axioms
  def output(self, stream):
    self.variables.output(stream)
    self.init.output(stream)
    self.goal.output(stream)
    print >> stream, len(self.operators)
    for op in self.operators:
      op.output(stream)
    print >> stream, len(self.axioms)
    for axiom in self.axioms:
      axiom.output(stream)

class SASVariables:
  def __init__(self, ranges, axiom_layers):
    self.ranges = ranges
    self.axiom_layers = axiom_layers
  def dump(self):
    for var, (rang, axiom_layer) in enumerate(zip(self.ranges, self.axiom_layers)):
      if axiom_layer != -1:
        axiom_str = " [axiom layer %d]" % axiom_layer
      else:
        axiom_str = ""
      print "v%d in {%s}%s" % (var, range(rang), axiom_str)
  def output(self, stream):
    print >> stream, "begin_variables"
    print >> stream, len(self.ranges)
    for var, (rang, axiom_layer) in enumerate(zip(self.ranges, self.axiom_layers)):
      print >> stream, "var%d %d %d" % (var, rang, axiom_layer)
    print >> stream, "end_variables"

class SASInit:
  def __init__(self, values):
    self.values = values
  def dump(self):
    for var, val in enumerate(self.values):
      if val != -1:
        print "v%d: %d" % (var, val)
  def output(self, stream):
    print >> stream, "begin_state"
    for val in self.values:
      print >> stream, val
    print >> stream, "end_state"

class SASGoal:
  def __init__(self, pairs):
    self.pairs = pairs[:]
    self.pairs.sort()
  def dump(self):
    for var, val in self.pairs:
      print "v%d: %d" % (var, val)
  def output(self, stream):
    print >> stream, "begin_goal"
    print >> stream, len(self.pairs)
    for var, val in self.pairs:
      print >> stream, var, val
    print >> stream, "end_goal"

class SASOperator:
  def __init__(self, name, prevail, pre_post):
    self.name = name
    self.prevail = prevail
    self.pre_post = pre_post
  def dump(self):
    print self.name
    print "Prevail:"
    for var, val in self.prevail:
      print "  v%d: %d" % (var, val)
    print "Pre/Post:"
    for var, pre, post, cond in self.pre_post:
      if cond:
        cond_str = " [%s]" % ", ".join(["%d: %d" % tuple(c) for c in cond])
      else:
        cond_str = ""
      print "  v%d: %d -> %d%s" % (var, pre, post, cond_str)
  def output(self, stream):
    print >> stream, "begin_operator"
    print >> stream, self.name[1:-1]
    print >> stream, len(self.prevail)
    for var, val in self.prevail:
      print >> stream, var, val
    print >> stream, len(self.pre_post)
    for var, pre, post, cond in self.pre_post:
      print >> stream, len(cond),
      for cvar, cval in cond:
        print >> stream, cvar, cval,
      print >> stream, var, pre, post
    print >> stream, "end_operator"

class SASAxiom:
  def __init__(self, condition, effect):
    self.condition = condition
    self.effect = effect
    assert self.effect[1] in (0, 1)

    for _, val in condition:
      assert val >= 0, condition
  def dump(self):
    print "Condition:"
    for var, val in self.condition:
      print "  v%d: %d" % (var, val)
    print "Effect:"
    var, val = self.effect
    print "  v%d: %d" % (var, val)
  def output(self, stream):
    print >> stream, "begin_rule"
    print >> stream, len(self.condition)
    for var, val in self.condition:
      print >> stream, var, val
    var, val = self.effect
    print >> stream, var, 1 - val, val
    print >> stream, "end_rule"

