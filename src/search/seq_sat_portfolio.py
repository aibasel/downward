# -*- coding: utf-8 -*-

import glob
import os
import os.path
import resource
import sys


DEFAULT_TIMEOUT = 1800


def safe_unlink(filename):
    try:
        os.unlink(filename)
    except EnvironmentError:
        pass

def adapt_search(args, extra_args, search_cost_type, heuristic_cost_type, plan_file):
    g_bound = "infinity"
    plan_no = 0
    try:
        for line in open("plan_numbers_and_cost"):
            plan_no += 1
            g_bound = int(line.split()[1])
    except IOError:
        pass
    for index, arg in enumerate(args):
        if arg == "--heuristic":
            heuristic_config = args[index + 1]
            heuristic_config = heuristic_config.replace("H_COST_TYPE",
                               str(heuristic_cost_type))
            args[index + 1] = heuristic_config
        elif arg == "--search":
            search_config = args[index + 1]
            if search_config.startswith("iterated"):
                extra_args[1] = plan_file
            else:
                extra_args[1] = "%s.%d" % (plan_file, plan_no + 1)
            search_config = search_config.replace("BOUND", str(g_bound))
            search_config = search_config.replace("PLANCOUNTER", str(plan_no))
            search_config = search_config.replace("H_COST_TYPE",
                               str(heuristic_cost_type))
            search_config = search_config.replace("S_COST_TYPE",
                               str(search_cost_type))
            args[index + 1] = search_config
            break
    return g_bound, plan_no

def run(nonunit_configs, unit_configs, final_config=None, timeout=DEFAULT_TIMEOUT):
    extra_args = sys.argv[1:]
    assert len(extra_args) == 4, extra_args
    assert extra_args[0] in ["unit", "nonunit"], extra_args
    unitcost = extra_args.pop(0)
    assert extra_args[0][-1] in ["1", "2", "4"], extra_args
    planner = extra_args.pop(0)
    assert extra_args[0] == "--plan-file", extra_args
    plan_file = extra_args[1]

    safe_unlink(plan_file)
    for filename in glob.glob("%s.*" % plan_file):
        safe_unlink(filename)
    safe_unlink("plan_numbers_and_cost")

    remaining_time_at_start = float(timeout)
    try:
        for line in open("elapsed.time"):
            if line.strip():
                remaining_time_at_start -= float(line)
    except EnvironmentError:
        print "WARNING! elapsed_time not found -- assuming full time available."

    print "remaining time at start: %s" % remaining_time_at_start

    if unitcost == "unit":
        configs = unit_configs
    else:
        configs = nonunit_configs

    restarted = False
    heuristic_cost_type = 1
    search_cost_type = 1
    while True:
        second_iteration = False
        if restarted:
            search_cost_type = 0
            heuristic_cost_type = 2
            second_iteration = True
        for pos, (relative_time, args) in enumerate(configs):
            args = list(args)
            g_bound, plan_no = adapt_search(args, extra_args, search_cost_type, 
                                            heuristic_cost_type, plan_file)
            print "g bound: %s" % g_bound
            print "next plan number: %d" % (plan_no + 1)
            remaining_time = remaining_time_at_start - sum(os.times()[:4])
            print "remaining time: %s" % remaining_time
            remaining_relative_time = sum(config[0] for config in configs[pos:])
            print "config %d: relative time %d, remaining %d" % (
                pos, relative_time, remaining_relative_time)
            run_timeout = remaining_time * relative_time / remaining_relative_time
            print "timeout: %.2f" % run_timeout
            complete_args = [planner] + args + extra_args
            print "args: %s" % complete_args
            sys.stdout.flush()
            if not os.fork():
                os.close(0)
                os.open("output", os.O_RDONLY)
                if relative_time != remaining_relative_time:
                    resource.setrlimit(resource.RLIMIT_CPU, (
                        int(run_timeout), int(run_timeout)))
                os.execl(planner, *complete_args)
            os.wait()

            curr_plan_file = "%s.%d" % (plan_file, plan_no + 1)
            if (unitcost != "unit" and not restarted and
                os.path.exists(curr_plan_file)):
                # found a plan -> restart with "true" costs
                restarted = True
                configs = configs[pos:] + configs[:pos]
                break
        if final_config and (second_iteration or unitcost == "unit"):
            break

    # run final config without time limit
    final_config = list(final_config)
    g_bound, plan_no = adapt_search(final_config, extra_args, search_cost_type,
                                    heuristic_cost_type, plan_file)
    print "g bound: %s" % g_bound
    print "next plan number: %d" % (plan_no + 1)
    complete_args = [planner] + final_config + extra_args
    print "args: %s" % complete_args
    sys.stdout.flush()
    if not os.fork():
        os.close(0)
        os.open("output", os.O_RDONLY)
        os.execl(planner, *complete_args)
    os.wait()
