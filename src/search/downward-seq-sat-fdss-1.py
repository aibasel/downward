#! /usr/bin/env python2.6
# -*- coding: utf-8 -*-

import os
import os.path
import resource
import sys

TIMEOUT = 1800

# NOTE: if there is a iterated search included, there must be the option
# "plan_counter=PLANCOUNTER"

NONUNIT_CONFIGS = [
    (91, ["--heuristic", "hff=ff(cost_type=1)", 
         "--heuristic", "hcg=cg(cost_type=1)", "--search", 
         "eager_greedy(hff,hcg,preferred=(hff,hcg),cost_type=1,bound=BOUND)"]),
    (89, ["--heuristic", "hadd=add(cost_type=1)", "--heuristic", 
        "hcg=cg(cost_type=1)", "--search", 
        "lazy_greedy(hadd,hcg,preferred=(hadd,hcg),cost_type=1,bound=BOUND)"]),
    (82, ["--heuristic", "hadd=add(cost_type=1)",
          "--heuristic", "hcg=cg(cost_type=1)",
          "--search",
          "eager_greedy(hadd,hcg,preferred=(hadd,hcg),cost_type=1,bound=BOUND)"]),
    (30, ["--heuristic", "h=cg(cost_type=1)",
          "--search",
          "eager_greedy(h,preferred=(h),cost_type=0,bound=BOUND)"]),
    (79, ["--heuristic", "h=cg(cost_type=1)",
          "--search",
          "eager(single(sum(g(),weight(h,3))),preferred=(h),cost_type=0,bound=BOUND)"]),
    (105, ["--heuristic", "h=ff(cost_type=1)",
          "--search",
          "eager(single(sum(g(),weight(h,3))),preferred=(h),cost_type=1,bound=BOUND)"]),
    (356, ["--heuristic", "h=cg(cost_type=1)",
          "--search",
          "lazy_greedy(h,preferred=(h),cost_type=0,bound=BOUND)"]),
    (104, ["--heuristic", "h=add(cost_type=1)",
          "--search",
          "lazy_wastar(h,w=3,preferred=(h),cost_type=0,bound=BOUND)"]),
    (37, ["--heuristic", "h=add(cost_type=1)",
          "--search",
          "lazy_wastar(h,w=3,preferred=(h),cost_type=1,bound=BOUND)"]),
    (93, ["--heuristic", "h=ff(cost_type=1)",
          "--search",
          "lazy_wastar(h,w=3,preferred=(h),cost_type=1,bound=BOUND)"]),
    (139, ["--heuristic", "h=ff(cost_type=1)", 
          "--search",
          "lazy_wastar(h,w=3,preferred=(h),cost_type=0,bound=BOUND)"]),
     ]

UNIT_CONFIGS = [
    (106, ["--heuristic", "hff=ff(cost_type=1)", 
          "--heuristic", "hadd=add(cost_type=1)",
          "--search",
          "eager_greedy(hff,hadd,preferred=(hff,hadd),cost_type=1,bound=BOUND)"]),
    (117, ["--heuristic", "hff=ff(cost_type=1)",
          "--heuristic", "hcea=cea(cost_type=1)", 
          "--heuristic", "hcg=cg(cost_type=1)",
          "--search",
          "eager_greedy(hff,hcea,hcg,preferred=(hff,hcea,hcg),cost_type=1,bound=BOUND)"]),
    (50, ["--heuristic", "hff=ff(cost_type=1)",
          "--heuristic", "hcea=cea(cost_type=1)",
          "--heuristic", "hcg=cg(cost_type=1)",
          "--search",
          "lazy_greedy(hff,hcea,hcg,preferred=(hff,hcea,hcg),cost_type=1,bound=BOUND)"]),
    (213, ["--heuristic", "h=cea(cost_type=1)",
          "--search",
          "eager_greedy(h,preferred=(h),cost_type=0,bound=BOUND)"]),
    (93, ["--heuristic", "h=cg(cost_type=1)",
          "--search",
          "eager_greedy(h,preferred=(h),cost_type=0,bound=BOUND)"]),
    (94, ["--heuristic", "h=add(cost_type=1)", 
          "--search",
          "eager(single(sum(g(),weight(h,3))),preferred=(h),cost_type=0,bound=BOUND)"]),
    (98, ["--heuristic", "h=ff(cost_type=1)",
         "--search",
         "eager(single(sum(g(),weight(h,3))),preferred=(h),cost_type=0,bound=BOUND)"]),
    (57, ["--heuristic", "h=cea(cost_type=1)", 
          "--search",
          "lazy_greedy(h,preferred=(h),cost_type=0,bound=BOUND)"]),
    (111, ["--heuristic", "h=ff(cost_type=1)",
          "--search",
          "lazy_greedy(h,preferred=(h),cost_type=0,bound=BOUND)"]),
    (65, ["--heuristic", "h=cea(cost_type=1)", 
         "--search",
         "lazy_wastar(h,w=3,preferred=(h),cost_type=0,bound=BOUND)"]),
    (88, ["--heuristic", "h=cg(cost_type=1)",
         "--search",
         "lazy_wastar(h,w=3,preferred=(h),cost_type=0,bound=BOUND)"]),
    (340, ["--heuristic", "h=ff(cost_type=1)",
         "--search",
         "lazy_wastar(h,w=3,preferred=(h),cost_type=0,bound=BOUND)"]),
     ]

extra_args = sys.argv[1:]
assert len(extra_args) == 4, extra_args
assert extra_args[0] in ["unit", "nonunit"], extra_args
unitcost = extra_args.pop(0)
assert extra_args[0][-1] in ["1", "2", "4"], extra_args
planner = extra_args.pop(0)
assert extra_args[0] == "--plan-file", extra_args
plan_file = extra_args[1]

# TODO should also unlink plan_file.1, plan_file.2, ...?
try:
    os.unlink(plan_file)
    os.unlink("plan_numbers_and_cost")
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

if unitcost == "unit":
    CONFIGS = UNIT_CONFIGS
else:
    CONFIGS = NONUNIT_CONFIGS

for pos, (relative_time, args) in enumerate(CONFIGS):
    g_bound = "infinity"
    plan_no = 0
    try:
        for line in open("plan_numbers_and_cost"):
            plan_no += 1
            g_bound = int(line.split()[1])
    except IOError:
        pass
    print "g bound: %s" % g_bound
    print "next plan number: %d" % (plan_no + 1)
    adapted_search = False
    for index, arg in enumerate(args):
        if arg == "--search":
            search_config = args[index + 1]
            if search_config.startswith("iterated"):
                extra_args[1] = plan_file
            else:
                extra_args[1] = "%s.%d" % (plan_file, plan_no + 1)
            search_config = search_config.replace("BOUND", str(g_bound))
            search_config = search_config.replace("PLANCOUNTER", str(plan_no))
            args[index + 1] = search_config
            adapted_search = True
            break
    assert adapted_search
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
