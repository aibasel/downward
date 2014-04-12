#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os.path
import subprocess
import sys


BASE_DIR = os.path.abspath(os.path.dirname(__file__))
TRANSLATE = os.path.join(BASE_DIR, "translate", "translate.py")
PREPROCESS = os.path.join(BASE_DIR, "preprocess", "preprocess")
SEARCH = os.path.join(BASE_DIR, "search", "downward")


def die(msg):
    raise SystemExit(msg)


def usage():
    die("usage: %s [DOMAIN_FILE] PROBLEM_FILE SEARCH_OPTION ..." %
        os.path.basename(__file__))


def parse_args():
    args = sys.argv[1:]
    if len(args) < 2:
        usage()

    if os.path.exists(args[1]):
        num_translator_args = 2
    else:
        num_translator_args = 1

    translator_args = args[:num_translator_args]
    search_args = args[num_translator_args:]
    return translator_args, search_args


def write_elapsed_time():
    ## Note: According to the os.times documentation, Windows sets the
    ## child time components to 0, so this won't work properly under
    ## Windows.
    ##
    ## TODO: Find a solution for this. A simple solution might be to
    ## just document this as a limitation under Windows that causes
    ## time slices for portfolios to be allocated slightly wrongly.
    ## Another solution would be to base time slices on wall-clock
    ## time under Windows.
    times = os.times()
    child_elapsed = times[2] + times[3]
    with open("elapsed.time", "w") as time_file:
        print >> time_file, child_elapsed


def call_cmd(msg, cmd, stdin=None):
    print msg
    sys.stdout.flush()
    if stdin:
        with open(stdin) as stdin_file:
            subprocess.check_call(cmd, stdin=stdin_file)
    else:
        subprocess.check_call(cmd)
    print


def main():
    translator_args, search_args = parse_args()
    print "translator arguments: %s" % translator_args
    print "search arguments: %s" % search_args
    print

    call_cmd("1. Running translator", [TRANSLATE] + list(translator_args))
    call_cmd("2. Running preprocessor", [PREPROCESS], stdin="output.sas")
    write_elapsed_time()
    call_cmd("3. Running search", [SEARCH] + list(search_args), stdin="output")

    ## TODO: The old "plan" script preserved the exit code of the
    ## search component. This code preserves exit code 0, but
    ## everything else leads to a traceback and exit code 1. It would
    ## not be hard to change this, but it is probably better to wait
    ## with this until the search/downward script and portfolio
    ## scripts are integrated.

if __name__ == "__main__":
    main()
