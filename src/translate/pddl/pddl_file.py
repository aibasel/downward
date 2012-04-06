#! /usr/bin/env python

import sys
import os.path
import re

import parser

import tasks

def parse_pddl_file(type, filename):
    try:
        return parser.parse_nested_list(file(filename))
    except IOError, e:
        raise SystemExit("Error: Could not read file: %s\nReason: %s." %
                         (e.filename, e))
    except parser.ParseError, e:
        raise SystemExit("Error: Could not parse %s file: %s\n" % (type, filename))

def open(task_filename=None, domain_filename=None):
    if task_filename is None:
        if len(sys.argv) not in (2, 3):
            raise SystemExit("Error: Need exactly one or two command line arguments.\n"
                             "Usage: %s [<domain.pddl>] <task.pddl>" % sys.argv[0])

        task_filename = sys.argv[-1]
        if len(sys.argv) == 3:
            domain_filename = sys.argv[1]

    if not domain_filename:
        dirname, basename = os.path.split(task_filename)
        domain_filename = os.path.join(dirname, "domain.pddl")
        if not os.path.exists(domain_filename) and re.match(r"p[0-9][0-9]\b", basename):
            domain_filename = os.path.join(dirname, basename[:4] + "domain.pddl")
        if not os.path.exists(domain_filename) and re.match(r"p[0-9][0-9]\b", basename):
            domain_filename = os.path.join(dirname, basename[:3] + "-domain.pddl")
        if not os.path.exists(domain_filename) and re.match(r"p[0-9][0-9]\b", basename):
            domain_filename = os.path.join(dirname, "domain_" + basename)
        if not os.path.exists(domain_filename) and basename.endswith("-problem.pddl"):
            domain_filename = os.path.join(dirname, basename[:-13] + "-domain.pddl")
        if not os.path.exists(domain_filename):
            raise SystemExit("Error: Could not find domain file using "
                             "automatic naming rules.")

    domain_pddl = parse_pddl_file("domain", domain_filename)
    task_pddl = parse_pddl_file("task", task_filename)
    return tasks.Task.parse(domain_pddl, task_pddl)

if __name__ == "__main__":
    open().dump()
