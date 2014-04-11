# -*- coding: utf-8 -*-

# Some performance numbers: on the largest preprocessor output file in
# our benchmark collection as of end of 2013 (satellite #33, ~104 MB),
# analyzing the input requires roughly 5 seconds. For comparison: the
# translator requires 130 seconds and the preprocessor 33 seconds on
# this task.


class ParseError(Exception):
    pass


class FileParser(object):
    """Simple generic file parser."""

    def __init__(self, input_file):
        self.input_file = input_file
        self.curr_line = None
        self.prev_line = None

    def error(self, msg):
        """Raise a ParseError with the given message."""
        raise ParseError("error in %s: %s" % (self.input_file.name, msg))

    def next_line(self):
        """Read the next line and return it. All input lines are stripped."""
        self.prev_line = self.curr_line
        try:
            self.curr_line = next(self.input_file).strip()
        except StopIteration:
            self.error("unexpected end of file")
        return self.curr_line

    def get_curr_line(self):
        """Get the current line, i.e., the one that was read by the
        last call to next_line()."""
        return self.curr_line

    def get_prev_line(self):
        """Get the previous line, i.e., the one that was read by the
        second-last call to next_line()."""
        return self.prev_line

    def skip_to(self, magic):
        """Skip over all input lines up to and including the next
        one that is equal to "magic"."""
        while self.next_line() != magic:
            pass

    def read_int(self, **kwargs):
        """Read the next line and parse it as an integer.
        (See parse_int for keyword arguments.)"""
        return self.parse_int(self.next_line(), **kwargs)

    def parse_int(self, line, min_value=None, max_value=None):
        """Parse a given line of a text as an integer and check that
        it is between the given bounds (if specified)."""
        try:
            value = int(line)
        except ValueError:
            self.error("expected int, got %r" % line)
        if ((min_value is not None and value < min_value) or
            (max_value is not None and value > max_value)):
            self.error("value out of bound: %d" % value)
        return value


def is_unit_cost(filename):
    with open(filename) as input_file:
        parser = FileParser(input_file)

        parser.skip_to("begin_metric")
        use_metric = bool(parser.read_int(min_value=0, max_value=1))

        parser.skip_to("end_goal")
        num_operators = parser.read_int(min_value=0)
        for _ in xrange(num_operators):
            parser.skip_to("begin_operator")
            # Skip over operator name in case it's "end_operator":
            parser.next_line()
            parser.skip_to("end_operator")
            action_cost = parser.parse_int(parser.get_prev_line(), min_value=0)
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
                    parser.error("action cost set despite use_metric=False")
        return True


def test():
    import sys
    args = sys.argv[1:]
    for filename in args:
        if len(args) >= 2:
            print "%s:" % filename,
        print "is_unit_cost:", is_unit_cost(filename)
        if len(args) >= 2:
            print


if __name__ == "__main__":
    test()
