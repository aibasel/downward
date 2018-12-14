#! /usr/bin/env python

import logging
import re

from lab.parser import Parser


class CommonParser(Parser):
    def add_difference(self, diff, val1, val2):
        def diff_func(content, props):
            if props.get(val1) is None or props.get(val2) is None:
                diff_val = None
            else:
                diff_val = props.get(val1) - props.get(val2)
            props[diff] = diff_val
        self.add_function(diff_func)

    def _get_flags(self, flags_string):
        flags = 0
        for char in flags_string:
            flags |= getattr(re, char)
        return flags

    def add_repeated_pattern(
            self, name, regex, file="run.log", required=True, type=int,
            flags=""):
        flags += "M"

        def find_all_occurences(content, props):
            matches = re.findall(regex, content, flags=self._get_flags(flags))
            if required and not matches:
                logging.error("Pattern {0} not found in file {1}".format(regex, file))
            props[name] = [type(m) for m in matches]

        self.add_function(find_all_occurences, file=file)

    def add_pattern(self, name, regex, file="run.log", required=False, type=int, flags="M"):
        Parser.add_pattern(self, name, regex, file=file, required=required, type=type, flags=flags)

    def add_bottom_up_pattern(self, name, regex, file="run.log", required=True, type=int, flags=""):

        def search_from_bottom(content, props):
            reversed_content = "\n".join(reversed(content.splitlines()))
            match = re.search(regex, reversed_content, flags=self._get_flags(flags))
            if required and not match:
                logging.error("Pattern {0} not found in file {1}".format(regex, file))
            if match:
                props[name] = type(match.group(1))

        self.add_function(search_from_bottom, file=file)


def no_search(content, props):
    if "search_start_time" not in props:
        error = props.get("error")
        if error is not None and error != "incomplete-search-found-no-plan":
            props["error"] = "no-search-due-to-" + error


def main():
    parser = CommonParser()
    parser.add_pattern("search_start_time", r"\[g=0, 1 evaluated, 0 expanded, t=(.+)s, \d+ KB\]", type=float, required=False)
    parser.add_pattern("search_start_memory", r"\[g=0, 1 evaluated, 0 expanded, t=.+s, (\d+) KB\]", type=int, required=False)
    parser.add_pattern("init_time", r"^Time for initializing additive Cartesian heuristic: (.+)s$", type=float, required=False)
    parser.add_pattern("cartesian_states", r"^Cartesian states: (\d+)$", type=int, required=False)

    parser.add_function(no_search)

    parser.parse()


if __name__ == "__main__":
    main()
