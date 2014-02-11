# -*- coding: utf-8 -*-

# Some performance numbers: on the largest preprocessor output file in
# our benchmark collection as of end of 2013 (satellite #33, ~104 MB),
# the input analyzer requires roughly 5.01 seconds for a complete
# analysis and 0.038 seconds to only check the variable ranges. For
# comparison: the translator requires 130 seconds and the preprocessor
# 33 seconds on this task.


class AnalysisError(Exception):
    pass


class InputAnalyzer(object):
    """Simple parser for the preprocessor output file. Not a complete
    parser, so does not completely validate that the input is
    syntax-correct.

    The class is intended to be used through the helper functions in
    the module rather than directly."""

    def analyze(self, filename, extract_func):
        self.filename = filename
        self.curr_line = None
        self.prev_line = None
        with open(filename) as input_file:
            self.input_file = input_file
            return extract_func()

    def extract_max_var_range(self):
        self._skip_to("end_metric")
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

    def extract_unit_cost(self):
        is_unit_cost = True

        self._skip_to("begin_metric")
        use_metric = bool(self._read_int(min_value=0, max_value=1))

        self._skip_to("end_goal")
        num_operators = self._read_int(min_value=0)
        for _ in xrange(num_operators):
            self._skip_to("begin_operator")
            self._next_line() # Skip over operator name in case it's "end_operator."
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


def read_max_var_range(filename):
    analyzer = InputAnalyzer()
    return analyzer.analyze(filename, analyzer.extract_max_var_range)


def read_state_var_size(filename):
    return get_state_var_size(read_max_var_range(filename))


def read_unit_cost(filename):
    analyzer = InputAnalyzer()
    return analyzer.analyze(filename, analyzer.extract_unit_cost)


def test():
    import sys
    args = sys.argv[1:]
    for filename in args:
        if len(args) >= 2:
            print "%s:" % filename
        print "is_unit_cost:", read_unit_cost(filename)
        max_var_range = read_max_var_range(filename)
        print "max_var_range:", max_var_range
        print "state_var_size:", get_state_var_size(max_var_range)
        if len(args) >= 2:
            print


if __name__ == "__main__":
    test()
