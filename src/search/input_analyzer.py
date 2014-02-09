# -*- coding: utf-8 -*-

class AnalysisError(Exception):
    pass


class AnalysisResult(object):
    """Used to store the result of an input file analysis. Has the
    attributes is_unit_cost, max_var_range and state_var_size."""

    def dump(self):
        print "is_unit_cost: %s" % self.is_unit_cost
        print "max_var_range: %d" % self.max_var_range
        print "state_var_size: %d" % self.state_var_size


class InputAnalyzer(object):
    """Simple parser for the preprocessor output file. Not a complete
    parser, so does not completely validate that the input is
    syntax-correct."""

    def analyze(self, filename):
        self.filename = filename
        self.curr_line = None
        self.prev_line = None

        result = AnalysisResult()
        with open(filename) as input_file:
            self.input_file = input_file
            use_metric = self._parse_metric()
            result.max_var_range = self._parse_variables()
            result.is_unit_cost = self._parse_actions(use_metric)
        result.state_var_size = get_state_var_size(result.max_var_range)
        return result

    def _error(self, msg):
        raise AnalysisError("error in %s: %s" % (self.filename, msg))

    def _next_line(self):
        self.prev_line = self.curr_line
        try:
            self.curr_line = next(self.input_file).strip()
        except StopIteration:
            self._error("unexpected end of file")
        return self.curr_line

    def _skip_to(self, magic):
        while self._next_line() != magic:
            pass

    def _parse_int(self, line, min_value=None, max_value=None):
        try:
            value = int(line)
        except ValueError:
            self._error("expected int, got %r" % line)
        if ((min_value is not None and value < min_value) or
            (max_value is not None and value > max_value)):
            self._error("value out of bound: %d" % value)
        return value

    def _read_int(self, **kwargs):
        return self._parse_int(self._next_line(), **kwargs)

    def _parse_metric(self):
        self._skip_to("begin_metric")
        use_metric = bool(self._read_int(min_value=0, max_value=1))
        self._skip_to("end_metric")
        return use_metric

    def _parse_variables(self):
        num_variables = self._read_int(min_value=1)
        max_range = 0
        for _ in xrange(num_variables):
            self._skip_to("begin_variable")
            self._next_line() # Skip variable name.
            self._next_line() # Skip axiom layer specification.
            var_range = self._read_int(min_value=1)
            max_range = max(max_range, var_range)
            self._skip_to("end_variable")
        return max_range

    def _parse_actions(self, use_metric):
        is_unit_cost = True
        self._skip_to("end_goal")
        num_operators = self._read_int(min_value=0)
        for _ in xrange(num_operators):
            self._skip_to("end_operator")
            action_cost = self._parse_int(self.prev_line, min_value=0)
            if use_metric:
                if action_cost != 1:
                    is_unit_cost = False
            else:
                if action_cost != 0:
                    # Currently, action costs in the input must all be
                    # specified as 0 unless the metric is set. In the
                    # future, it might make sense to change the
                    # entries to 1 in this case (since they are
                    # interpreted as 1) and get rid of the metric
                    # block.
                    self._error("action cost set despite use_metric=False")
        return is_unit_cost


def get_state_var_size(max_range):
    # Note that the highest value in the state_var_t domain is
    # reserved for special purposes, so we can e.g. only fit 255
    # different valid values into an unsigned char.
    if max_range <= 255:
        return 1
    elif max_range <= 65535:
        return 2
    else:
        return 4


def analyze(filename):
    return InputAnalyzer().analyze(filename)


def test():
    import sys
    args = sys.argv[1:]
    for filename in args:
        if len(args) >= 2:
            print "%s:" % filename
        analyze(filename).dump()
        if len(args) >= 2:
            print


if __name__ == "__main__":
    test()
