# -*- coding: utf-8 -*-

import glob
import math
import optparse
import os
import os.path
import resource
import signal
import subprocess
import sys


DEFAULT_TIMEOUT = 1800
# On maia the python process that runs this module reserves about 128MB of
# virtual memory. To make it reserve less space, it is necessary to lower the
# soft limit for virtual memory before the process is started. This is done in
# the "downward" wrapper script. In order not to reach the external memory limit
# during a planner run, we don't allow the planner to use the reserved memory.
BYTES_FOR_PYTHON = 50 * 1024 * 1024

# Exit codes.
EXIT_PLAN_FOUND = 0
EXIT_CRITICAL_ERROR = 1
EXIT_INPUT_ERROR = 2
EXIT_UNSUPPORTED = 3
EXIT_UNSOLVABLE = 4
EXIT_UNSOLVED_INCOMPLETE = 5
EXIT_OUT_OF_MEMORY = 6
EXIT_TIMEOUT = 7
EXIT_TIMEOUT_AND_MEMORY = 8
EXIT_SIGXCPU = -signal.SIGXCPU

EXPECTED_EXITCODES = set([
    EXIT_PLAN_FOUND, EXIT_UNSOLVABLE, EXIT_UNSOLVED_INCOMPLETE,
    EXIT_OUT_OF_MEMORY, EXIT_TIMEOUT])


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

def set_limit(kind, soft, hard):
    try:
        resource.setrlimit(kind, (soft, hard))
    except (OSError, ValueError), err:
        # This can happen if the limit has already been set externally.
        sys.stderr.write("Limit for %s could not be set to %s (%s). "
                         "Previous limit: %s\n" %
                         (kind, (soft, hard), err, resource.getrlimit(kind)))

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

def run_search(planner, args, sas_file, plan_file, timeout=None, memory=None):
    complete_args = [planner] + args + ["--plan-file", plan_file]
    print "args: %s" % complete_args
    print "timeout: %.2f" % timeout
    sys.stdout.flush()

    def set_limits():
        if timeout is not None:
            # Don't try to raise the hard limit.
            _, external_hard_limit = resource.getrlimit(resource.RLIMIT_CPU)
            hard_limit = min(int(math.ceil(timeout)) + 1, external_hard_limit)
            # Soft limit reached --> SIGXCPU.
            # Hard limit reached --> SIGKILL.
            set_limit(resource.RLIMIT_CPU, hard_limit - 1, hard_limit)
        if memory is not None:
            # Memory in Bytes
            set_limit(resource.RLIMIT_AS, memory, memory)
        else:
            set_limit(resource.RLIMIT_AS, -1, -1)

    returncode = subprocess.call(complete_args, stdin=open(sas_file),
                                 preexec_fn=set_limits)
    print "returncode:", returncode
    print
    return returncode

def determine_timeout(remaining_time_at_start, configs, pos):
    remaining_time = remaining_time_at_start - sum(os.times()[:4])
    relative_time = configs[pos][0]
    print "remaining time: %s" % remaining_time
    remaining_relative_time = sum(config[0] for config in configs[pos:])
    print "config %d: relative time %d, remaining %d" % (
        pos, relative_time, remaining_relative_time)
    # For the last config we have relative_time == remaining_relative_time, so
    # we use all of the remaining time at the end.
    run_timeout = remaining_time * relative_time / remaining_relative_time
    return run_timeout

def _generate_exitcode(exitcodes):
    print "Exit codes:", exitcodes
    exitcodes = set(exitcodes)
    if EXIT_SIGXCPU in exitcodes:
        exitcodes.remove(EXIT_SIGXCPU)
        exitcodes.add(EXIT_TIMEOUT)
    unexpected_codes = exitcodes - EXPECTED_EXITCODES
    if unexpected_codes:
        print "Error: Unexpected exit codes:", list(unexpected_codes)
        return EXIT_CRITICAL_ERROR
    for code in [EXIT_PLAN_FOUND, EXIT_UNSOLVABLE, EXIT_UNSOLVED_INCOMPLETE]:
        if code in exitcodes:
            return code
    for code in [EXIT_OUT_OF_MEMORY, EXIT_TIMEOUT]:
        if exitcodes == set([code]):
            return code
    if exitcodes == set([EXIT_OUT_OF_MEMORY, EXIT_TIMEOUT]):
        return EXIT_TIMEOUT_AND_MEMORY
    print "Error: Unhandled exit codes:", exitcodes
    return EXIT_CRITICAL_ERROR

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

    assert len(extra_args) == 3, extra_args
    sas_file = extra_args.pop(0)
    assert extra_args[0] in ["unit", "nonunit"], extra_args
    unitcost = extra_args.pop(0)
    assert extra_args[0][-1] in ["1", "2", "4"], extra_args
    planner = extra_args.pop(0)

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
        exitcodes = run_opt(configs, planner, sas_file, plan_file,
                            remaining_time_at_start, memory)
    else:
        exitcodes = run_sat(configs, unitcost, planner, sas_file, plan_file,
                            final_config, final_config_builder,
                            remaining_time_at_start, memory)
    sys.exit(_generate_exitcode(exitcodes))

def _can_change_cost_type(args):
    return any('S_COST_TYPE' in part or 'H_COST_TYPE' in part for part in args)

def _run_sat_config(configs, pos, search_cost_type, heuristic_cost_type, planner,
                    sas_file, plan_file, remaining_time_at_start, memory):
    args = list(configs[pos][1])
    curr_plan_file = adapt_search(args, search_cost_type,
                                  heuristic_cost_type, plan_file)
    run_timeout = determine_timeout(remaining_time_at_start, configs, pos)
    if run_timeout <= 0:
        return None
    return run_search(planner, args, sas_file, curr_plan_file, run_timeout,
                      memory)

def run_sat(configs, unitcost, planner, sas_file, plan_file, final_config,
            final_config_builder, remaining_time_at_start, memory):
    exitcodes = []
    # For non-unitcost tasks we start by treating all costs as one. When we find
    # a solution, we rerun the successful config with real costs.
    heuristic_cost_type = 1
    search_cost_type = 1
    changed_cost_types = False
    while configs:
        configs_next_round = []
        for pos, (relative_time, args) in enumerate(configs):
            args = list(args)
            exitcode = _run_sat_config(configs, pos, search_cost_type,
                                       heuristic_cost_type, planner, sas_file,
                                       plan_file, remaining_time_at_start,
                                       memory)
            if exitcode is None:
                return exitcodes

            exitcodes.append(exitcode)
            if exitcode == EXIT_UNSOLVABLE:
                return exitcodes

            if exitcode == EXIT_PLAN_FOUND:
                configs_next_round.append(configs[pos][:])
                if (not changed_cost_types and unitcost != "unit" and
                        _can_change_cost_type(args)):
                    # Switch to real cost and repeat last run.
                    changed_cost_types = True
                    search_cost_type = 0
                    heuristic_cost_type = 2
                    exitcode = _run_sat_config(configs, pos, search_cost_type,
                                               heuristic_cost_type, planner,
                                               sas_file, plan_file,
                                               remaining_time_at_start, memory)
                    if exitcode is None:
                        return exitcodes

                    exitcodes.append(exitcode)
                    if exitcode == EXIT_UNSOLVABLE:
                        return exitcodes
                if final_config_builder:
                    print "Build final config."
                    final_config = final_config_builder(args[:])
                    break

        if final_config:
            break

        # Only run the successful configs in the next round.
        configs = configs_next_round

    if final_config:
        print "Abort portfolio and run final config."
        exitcode = _run_sat_config([(1, list(final_config))], 0, search_cost_type,
                                   heuristic_cost_type, planner, sas_file,
                                   plan_file, remaining_time_at_start, memory)
        if exitcode is not None:
            exitcodes.append(exitcode)
    return exitcodes

def run_opt(configs, planner, sas_file, plan_file, remaining_time_at_start,
            memory):
    exitcodes = []
    for pos, (relative_time, args) in enumerate(configs):
        timeout = determine_timeout(remaining_time_at_start, configs, pos)
        exitcode = run_search(planner, args, sas_file, plan_file, timeout, memory)
        exitcodes.append(exitcode)

        if exitcode in [EXIT_PLAN_FOUND, EXIT_UNSOLVABLE]:
            break
    return exitcodes
