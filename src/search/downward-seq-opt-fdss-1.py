#! /usr/bin/env python2.6
# -*- coding: utf-8 -*-

import os
import os.path
import resource
import sys

TIMEOUT = 1800

CONFIGS = [
    (175, ["--search",
           "astar(mas(max_states=1,merge_strategy=5,shrink_strategy=12))"]),
    (432, ["--search",
           "astar(mas(max_states=200000,merge_strategy=5,shrink_strategy=7))"]),
    (455, ["--search",
           "astar(lmcount(lm_merged(lm_rhw(),lm_hm(m=1)),admissible=true),mpd=true)"]),
    (569, ["--search",
           "astar(lmcut())"]),
     ]

extra_args = sys.argv[1:]
assert len(extra_args) == 3, extra_args
assert extra_args[0][-1] in ["1", "2", "4"], extra_args
planner = extra_args.pop(0)
assert extra_args[0] == "--plan-file", extra_args
plan_file = extra_args[1]

try:
    os.unlink(plan_file)
except EnvironmentError:
    pass

remaining_time_at_start = float(TIMEOUT)
try:
    for line in open("elapsed.time"):
        if line.strip():
            remaining_time_at_start -= float(line)
except EnvironmentError:
    print "WARNING! elapsed_time not found -- assuming full time available."

print "remaining time at start: %s" % remaining_time_at_start

for pos, (relative_time, args) in enumerate(CONFIGS):
    remaining_time = remaining_time_at_start - sum(os.times()[:4])
    print "remaining time: %s" % remaining_time
    remaining_relative_time = sum(config[0] for config in CONFIGS[pos:])
    print "config %d: relative time %d, remaining %d" % (
        pos, relative_time, remaining_relative_time)
    timeout = remaining_time * relative_time / remaining_relative_time
    print "timeout: %.2f" % timeout
    complete_args = [planner] + args + extra_args
    print "args: %s" % complete_args
    sys.stdout.flush()
    if not os.fork():
        os.close(0)
        os.open("output", os.O_RDONLY)
        resource.setrlimit(resource.RLIMIT_CPU, (int(timeout), int(timeout)))
        os.execl(planner, *complete_args)
    os.wait()
    if os.path.exists(plan_file):
        print "Plan found!"
        break
