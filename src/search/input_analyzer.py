# -*- coding: utf-8 -*-

# Some performance numbers: on the largest preprocessor output file in
# our benchmark collection as of end of 2013 (satellite #33, ~104 MB),
# the input analyzer requires roughly 5 seconds. For comparison: the
# translator requires 130 seconds and the preprocessor 33 seconds on
# this task.


class AnalysisError(Exception):
    pass


class UnitCostAnalyzer(object):
    """Simple parser for the preprocessor output file that checks if
    we have a unit-cost problem. Not a complete parser, so does not
    completely validate that the input is syntax-correct.

    The class is intended to be used through the helper functions in
    the module rather than directly."""

    def is_unit_cost(self, filename):
        # This is more general than needed, since we only use one
        # extract_func these days. (There used to be two different
        # ones back when we had the downward-{1,2,4} executables.)
        # But it doesn't hurt much to leave the generality in.
        self.filename = filename
        self.curr_line = None
        self.prev_line = None

        with open(filename) as input_file:
            self.input_file = input_file

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
                        return False
                else:
                    if action_cost != 0:
                        # Currently, action costs in the input must all be
                        # specified as 0 unless the metric is set. In the
                        # future, it might make sense to change the
                        # entries to 1 in this case (since they are
                        # interpreted as 1) and get rid of the metric
                        # block.
                        self._error("action cost set despite use_metric=False")

        return True

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


def read_unit_cost(filename):
    return UnitCostAnalyzer().is_unit_cost(filename)


def test():
    import sys
    args = sys.argv[1:]
    for filename in args:
        if len(args) >= 2:
            print "%s:" % filename
        print "is_unit_cost:", read_unit_cost(filename)
        if len(args) >= 2:
            print


if __name__ == "__main__":
    test()
