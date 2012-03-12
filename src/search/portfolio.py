# -*- coding: utf-8 -*-

import glob
import optparse
import os
import os.path
import resource
import sys


DEFAULT_TIMEOUT = 1800
# Measurements show that this process uses about 35 MB of virtual memory.
BYTES_FOR_PYTHON = 50 * 1024 * 1024

def parse_args():
    parser = optparse.OptionParser()
    parser.add_option("--plan-file", default="sas_plan",
                      help="Filename for the found plans (default: %default)")
    return parser.parse_args()

def safe_unlink(filename):
    try:
        os.unlink(filename)
    except EnvironmentError:
        pass

def set_limit(kind, amount):
    try:
        resource.setrlimit(kind, (amount, amount))
    except (OSError, ValueError), err:
        # This can happen if the limit has already been set externally.
        sys.stderr.write("Limit for %s could not be set to %s (%s). "
                         "Previous limit: %s\n" %
                         (kind, amount, err, resource.getrlimit(kind)))

def adapt_search(args, search_cost_type, heuristic_cost_type, plan_file):
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
                if not "plan_counter=PLANCOUNTER" in search_config:
                    raise ValueError("When using iterated search, we must add "
                                     "the option plan_counter=PLANCOUNTER")
                curr_plan_file = plan_file
            else:
                curr_plan_file = "%s.%d" % (plan_file, plan_no + 1)
            search_config = search_config.replace("BOUND", str(g_bound))
            search_config = search_config.replace("PLANCOUNTER", str(plan_no))
            search_config = search_config.replace("H_COST_TYPE",
                               str(heuristic_cost_type))
            search_config = search_config.replace("S_COST_TYPE",
                               str(search_cost_type))
            args[index + 1] = search_config
            break
    print "g bound: %s" % g_bound
    print "next plan number: %d" % (plan_no + 1)
    return curr_plan_file

def run_search(planner, args, plan_file, timeout=None, memory=None):
    complete_args = [planner] + args + ["--plan-file", plan_file]
    print "args: %s" % complete_args
    sys.stdout.flush()
    if not os.fork():
        os.close(0)
        os.open("output", os.O_RDONLY)
        if timeout:
            set_limit(resource.RLIMIT_CPU, int(timeout))
        if memory:
            # Memory in Bytes
            set_limit(resource.RLIMIT_AS, int(memory))
        os.execl(planner, *complete_args)
    os.wait()

def determine_timeout(remaining_time_at_start, configs, pos):
    remaining_time = remaining_time_at_start - sum(os.times()[:4])
    relative_time = configs[pos][0]
    print "remaining time: %s" % remaining_time
    remaining_relative_time = sum(config[0] for config in configs[pos:])
    print "config %d: relative time %d, remaining %d" % (
        pos, relative_time, remaining_relative_time)
    run_timeout = remaining_time * relative_time / remaining_relative_time
    print "timeout: %.2f" % run_timeout
    return run_timeout

def run(configs, optimal=True, final_config=None, final_config_builder=None,
        timeout=None):
    options, extra_args = parse_args()
    plan_file = options.plan_file

    # Time limits are either positive values in seconds or -1 (unlimited).
    soft_time_limit, hard_time_limit = resource.getrlimit(resource.RLIMIT_CPU)
    print 'External time limit:', hard_time_limit
    if (hard_time_limit >= 0 and timeout is not None and
        timeout != hard_time_limit):
        sys.stderr.write("The externally set timeout (%d) differs from the one "
                         "in the portfolio file (%d). Is this expected?\n" %
                         (hard_time_limit, timeout))
    # Prefer limits in the order: externally set, from portfolio file, default.
    if hard_time_limit >= 0:
        timeout = hard_time_limit
    elif timeout is None:
        sys.stderr.write("No timeout has been set for the portfolio so we take "
                         "the default of %ds.\n" % DEFAULT_TIMEOUT)
        timeout = DEFAULT_TIMEOUT
    print 'Internal time limit:', timeout

    # Memory limits are either positive values in Bytes or -1 (unlimited).
    soft_mem_limit, hard_mem_limit = resource.getrlimit(resource.RLIMIT_AS)
    print 'External memory limit:', hard_mem_limit
    memory = hard_mem_limit - BYTES_FOR_PYTHON
    # Do not limit memory if the previous limit was very low or unlimited.
    if memory < 0:
        memory = None
    print 'Internal memory limit:', memory

    assert len(extra_args) == 2, extra_args
    assert extra_args[0] in ["unit", "nonunit"], extra_args
    unitcost = extra_args.pop(0)
    assert extra_args[0][-1] in ["1", "2", "4"], extra_args
    planner = extra_args.pop(0)

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

    if optimal:
        run_opt(configs, planner, plan_file, remaining_time_at_start, memory)
    else:
        run_sat(configs, unitcost, planner, plan_file, final_config,
                final_config_builder, remaining_time_at_start, memory)

def run_sat(configs, unitcost, planner, plan_file, final_config,
            final_config_builder, remaining_time_at_start, memory):
    heuristic_cost_type = 1
    search_cost_type = 1
    changed_cost_types = False
    while True:
        for pos, (relative_time, args) in enumerate(configs):
            args = list(args)
            curr_plan_file = adapt_search(args, search_cost_type,
                                          heuristic_cost_type, plan_file)
            run_timeout = determine_timeout(remaining_time_at_start,
                                            configs, pos)
            run_search(planner, args, curr_plan_file, run_timeout, memory)

            if os.path.exists(curr_plan_file):
                # found a plan in last run
                if not changed_cost_types and unitcost != "unit":
                    # switch to real cost and repeat last run
                    changed_cost_types = True
                    search_cost_type = 0
                    heuristic_cost_type = 2
                    # TODO: refactor: thou shalt not copy code!
                    args = list(configs[pos][1])
                    curr_plan_file = adapt_search(args, search_cost_type,
                                                heuristic_cost_type, plan_file)
                    run_timeout = determine_timeout(remaining_time_at_start,
                                                    configs, pos)
                    run_search(planner, args, curr_plan_file, run_timeout,
                               memory)
                if final_config_builder:
                    # abort scheduled portfolio and start final config
                    args = list(configs[pos][1])
                    final_config = final_config_builder(args)
                    break

        if final_config:
            break

    # run final config without time limit
    final_config = list(final_config)
    curr_plan_file = adapt_search(final_config, search_cost_type,
                                  heuristic_cost_type, plan_file)
    run_search(planner, final_config, curr_plan_file)

def run_opt(configs, planner, plan_file, remaining_time_at_start, memory):
    for pos, (relative_time, args) in enumerate(configs):
        if pos == len(configs) - 1:
            # Do not add timeout for last config.
            timeout = None
        else:
            timeout = determine_timeout(remaining_time_at_start, configs, pos)
        run_search(planner, args, plan_file, timeout, memory)

        if os.path.exists(plan_file):
            print "Plan found!"
            break
