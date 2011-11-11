SAS_FILE_VERSION = 3


class SASTask:
    def __init__(self, variables, mutexes, init, goal,
                 operators, axioms, metric):
        self.variables = variables
        self.mutexes = mutexes
        self.init = init
        self.goal = goal
        self.operators = operators
        self.axioms = axioms
        self.metric = metric
    def output(self, stream):
        print >> stream, "begin_version"
        print >> stream, SAS_FILE_VERSION
        print >> stream, "end_version"
        print >> stream, "begin_metric"
        print >> stream, int(self.metric)
        print >> stream, "end_metric"
        self.variables.output(stream)
        print >> stream, len(self.mutexes)
        for mutex in self.mutexes:
            mutex.output(stream)
        self.init.output(stream)
        self.goal.output(stream)
        print >> stream, len(self.operators)
        for op in self.operators:
            op.output(stream)
        print >> stream, len(self.axioms)
        for axiom in self.axioms:
            axiom.output(stream)
    def get_encoding_size(self):
        task_size = 0
        task_size += self.variables.get_encoding_size()
        for mutex in self.mutexes:
            task_size += mutex.get_encoding_size()
        task_size += self.goal.get_encoding_size()
        for op in self.operators:
            task_size += op.get_encoding_size()
        for axiom in self.axioms:
            task_size += axiom.get_encoding_size()
        return task_size

class SASVariables:
    def __init__(self, ranges, axiom_layers, value_names):
        self.ranges = ranges
        self.axiom_layers = axiom_layers
        self.value_names = value_names
    def dump(self):
        for var, (rang, axiom_layer) in enumerate(zip(self.ranges, self.axiom_layers)):
            if axiom_layer != -1:
                axiom_str = " [axiom layer %d]" % axiom_layer
            else:
                axiom_str = ""
            print "v%d in {%s}%s" % (var, range(rang), axiom_str)
    def output(self, stream):
        print >> stream, len(self.ranges)
        for var, (rang, axiom_layer, values) in enumerate(zip(
                self.ranges, self.axiom_layers, self.value_names)):
            print >> stream, "begin_variable"
            print >> stream, "var%d" % var
            print >> stream, axiom_layer
            print >> stream, rang
            assert rang == len(values), (rang, values)
            for value in values:
                print >> stream, value
            print >> stream, "end_variable"
    def get_encoding_size(self):
        # A variable with range k has encoding size k + 1 to also give the
        # variable itself some weight.
        return len(self.ranges) + sum(self.ranges)

class SASMutexGroup:
    def __init__(self, facts):
        self.facts = facts
    def dump(self):
        for var, val in self.facts:
            print "v%d: %d" % (var, val)
    def output(self, stream):
        print >> stream, "begin_mutex_group"
        print >> stream, len(self.facts)
        for var, val in self.facts:
            print >> stream, var, val
        print >> stream, "end_mutex_group"
    def get_encoding_size(self):
        return len(self.facts)

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
        self.pairs = sorted(pairs)
    def dump(self):
        for var, val in self.pairs:
            print "v%d: %d" % (var, val)
    def output(self, stream):
        print >> stream, "begin_goal"
        print >> stream, len(self.pairs)
        for var, val in self.pairs:
            print >> stream, var, val
        print >> stream, "end_goal"
    def get_encoding_size(self):
        return len(self.pairs)

class SASOperator:
    def __init__(self, name, prevail, pre_post, cost):
        self.name = name
        self.prevail = sorted(prevail)
        self.pre_post = sorted(pre_post)
        self.cost = cost
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
        print >> stream, self.cost
        print >> stream, "end_operator"
    def get_encoding_size(self):
        size = 1 + len(self.prevail)
        for var, pre, post, cond in self.pre_post:
            size += 1 + len(cond)
            if pre != -1:
                size += 1
        return size

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
    def get_encoding_size(self):
        return 1 + len(self.condition)
