# -*- coding: utf-8 -*-
from __future__ import print_function

__all__ = ["run"]

# TODO: Rename portfolio.py to portfolios.py ?

import itertools
import math
import os
import re
import resource
import signal
import subprocess
import sys


DEFAULT_TIMEOUT = 1800
# In order not to reach the external memory limit during a planner run,
# we reserve some memory for the Python process (see also main.py).
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

# The portfolio's exitcode is determined as follows:
# There is exactly one type of unexpected exit code -> use it.
# There are multiple types of unexpected exit codes -> EXIT_CRITICAL_ERROR.
# [..., EXIT_PLAN_FOUND, ...] -> EXIT_PLAN_FOUND
# [..., EXIT_UNSOLVABLE, ...] -> EXIT_UNSOLVABLE
# [..., EXIT_UNSOLVED_INCOMPLETE, ...] -> EXIT_UNSOLVED_INCOMPLETE
# [..., EXIT_OUT_OF_MEMORY, ..., EXIT_TIMEOUT, ...] -> EXIT_TIMEOUT_AND_MEMORY
# [..., EXIT_TIMEOUT, ...] -> EXIT_TIMEOUT
# [..., EXIT_OUT_OF_MEMORY, ...] -> EXIT_OUT_OF_MEMORY


def set_limit(kind, soft, hard):
    try:
        resource.setrlimit(kind, (soft, hard))
    except (OSError, ValueError), err:
        # This can happen if the limit has already been set externally.
        print("Limit for %s could not be set to %s (%s). Previous limit: %s" %
              (kind, (soft, hard), err, resource.getrlimit(kind)), file=sys.stderr)


def get_plan_cost(sas_plan_file):
    with open(sas_plan_file) as input_file:
        for line in input_file:
            match = re.match(r"; cost = (\d+)\n", line)
            if match:
                return int(match.group(1))
    os.remove(sas_plan_file)
    print("Could not retrieve plan cost from %s. Deleted the file." % sas_plan_file)
    return None


def get_plan_file(prefix, number):
    return "%s.%d" % (prefix, number)


def get_g_bound_and_number_of_plans(plan_file):
    plan_costs = []
    for index in itertools.count(start=1):
        sas_plan_file = get_plan_file(plan_file, index)
        if os.path.exists(sas_plan_file):
            plan_cost = get_plan_cost(sas_plan_file)
            if plan_cost is not None:
                if plan_costs and not plan_costs[-1] > plan_cost:
                    raise SystemExit(
                        "Plan costs must decrease: %s" %
                        " -> ".join(str(c) for c in plan_costs + [plan_cost]))
                plan_costs.append(plan_cost)
        else:
            break
    bound = min(plan_costs) if plan_costs else "infinity"
    return bound, len(plan_costs)


def adapt_search(args, search_cost_type, heuristic_cost_type, plan_file):
    g_bound, plan_no = get_g_bound_and_number_of_plans(plan_file)
    for index, arg in enumerate(args):
        if arg == "--heuristic":
            heuristic = args[index + 1]
            heuristic = heuristic.replace("H_COST_TYPE", str(heuristic_cost_type))
            args[index + 1] = heuristic
        elif arg == "--search":
            search = args[index + 1]
            if search.startswith("iterated"):
                if "plan_counter=PLANCOUNTER" not in search:
                    raise ValueError("When using iterated search, we must add "
                                     "the option plan_counter=PLANCOUNTER")
                curr_plan_file = plan_file
            else:
                curr_plan_file = get_plan_file(plan_file, plan_no + 1)
            for name, value in [
                    ("BOUND", g_bound),
                    ("PLANCOUNTER", plan_no),
                    ("H_COST_TYPE", heuristic_cost_type),
                    ("S_COST_TYPE", search_cost_type)]:
                search = search.replace(name, str(value))
            args[index + 1] = search
            break
    print("g bound: %s" % g_bound)
    print("next plan number: %d" % (plan_no + 1))
    return curr_plan_file


def run_search(executable, args, sas_file, curr_plan_file, timeout=None, memory=None):
    complete_args = [executable] + args + ["--plan-file", curr_plan_file]
    print("args: %s" % complete_args)
    sys.stdout.flush()

    def set_limits():
        if timeout is not None:
            # Don't try to raise the hard limit.
            _, external_hard_limit = resource.getrlimit(resource.RLIMIT_CPU)
            if external_hard_limit == resource.RLIM_INFINITY:
                external_hard_limit = float("inf")
            # Soft limit reached --> SIGXCPU.
            # Hard limit reached --> SIGKILL.
            soft_limit = int(math.ceil(timeout))
            hard_limit = min(soft_limit + 1, external_hard_limit)
            print("timeout: %.2f -> (%d, %d)" % (timeout, soft_limit, hard_limit))
            sys.stdout.flush()
            set_limit(resource.RLIMIT_CPU, soft_limit, hard_limit)
        if memory is not None:
            # Memory in Bytes
            set_limit(resource.RLIMIT_AS, memory, memory)
        else:
            set_limit(resource.RLIMIT_AS, -1, -1)

    with open(sas_file) as input_file:
        returncode = subprocess.call(complete_args, stdin=input_file,
                                     preexec_fn=set_limits)
    print("returncode: %d" % returncode)
    print()
    return returncode


def get_elapsed_time():
    ## Note: According to the os.times documentation, Windows sets the
    ## child time components to 0, so this won't work properly under
    ## Windows.
    ##
    ## TODO: Find a solution for this. A simple solution might be to
    ## just document this as a limitation under Windows that causes
    ## time slices for portfolios to be allocated slightly wrongly.
    ## Another solution would be to base time slices on wall-clock
    ## time under Windows.
    return sum(os.times()[:4])


def determine_timeout(remaining_time_at_start, configs, pos):
    remaining_time = remaining_time_at_start - get_elapsed_time()
    relative_time = configs[pos][0]
    print("remaining time: %s" % remaining_time)
    remaining_relative_time = sum(config[0] for config in configs[pos:])
    print("config %d: relative time %d, remaining %d" %
          (pos, relative_time, remaining_relative_time))
    # For the last config we have relative_time == remaining_relative_time, so
    # we use all of the remaining time at the end.
    run_timeout = remaining_time * relative_time / remaining_relative_time
    return run_timeout


def run_sat_config(configs, pos, search_cost_type, heuristic_cost_type,
                   executable, sas_file, plan_file, remaining_time_at_start,
                   memory):
    args = list(configs[pos][1])
    curr_plan_file = adapt_search(args, search_cost_type,
                                  heuristic_cost_type, plan_file)
    run_timeout = determine_timeout(remaining_time_at_start, configs, pos)
    if run_timeout <= 0:
        return None
    return run_search(executable, args, sas_file, curr_plan_file, run_timeout,
                      memory)


def run_sat(configs, unitcost, executable, sas_file, plan_file, final_config,
            final_config_builder, remaining_time_at_start, memory):
    exitcodes = []
    # If the configuration contains S_COST_TYPE or H_COST_TYPE and the task
    # has non-unit costs, we start by treating all costs as one. When we find
    # a solution, we rerun the successful config with real costs.
    heuristic_cost_type = 1
    search_cost_type = 1
    changed_cost_types = False
    while configs:
        configs_next_round = []
        for pos, (relative_time, args) in enumerate(configs):
            args = list(args)
            exitcode = run_sat_config(
                configs, pos, search_cost_type, heuristic_cost_type,
                executable, sas_file, plan_file, remaining_time_at_start,
                memory)
            if exitcode is None:
                return exitcodes

            exitcodes.append(exitcode)
            if exitcode == EXIT_UNSOLVABLE:
                return exitcodes

            if exitcode == EXIT_PLAN_FOUND:
                configs_next_round.append(configs[pos][:])
                if (not changed_cost_types and unitcost != "unit" and
                        can_change_cost_type(args)):
                    # Switch to real cost and repeat last run.
                    changed_cost_types = True
                    search_cost_type = 0
                    heuristic_cost_type = 2
                    exitcode = run_sat_config(
                        configs, pos, search_cost_type, heuristic_cost_type,
                        executable, sas_file, plan_file,
                        remaining_time_at_start, memory)
                    if exitcode is None:
                        return exitcodes

                    exitcodes.append(exitcode)
                    if exitcode == EXIT_UNSOLVABLE:
                        return exitcodes
                if final_config_builder:
                    print("Build final config.")
                    final_config = final_config_builder(args[:])
                    break

        if final_config:
            break

        # Only run the successful configs in the next round.
        configs = configs_next_round

    if final_config:
        print("Abort portfolio and run final config.")
        exitcode = run_sat_config(
            [(1, list(final_config))], 0, search_cost_type,
            heuristic_cost_type, executable, sas_file, plan_file,
            remaining_time_at_start, memory)
        if exitcode is not None:
            exitcodes.append(exitcode)
    return exitcodes


def run_opt(configs, executable, sas_file, plan_file, remaining_time_at_start,
            memory):
    exitcodes = []
    for pos, (relative_time, args) in enumerate(configs):
        timeout = determine_timeout(remaining_time_at_start, configs, pos)
        exitcode = run_search(executable, args, sas_file, plan_file, timeout, memory)
        exitcodes.append(exitcode)

        if exitcode in [EXIT_PLAN_FOUND, EXIT_UNSOLVABLE]:
            break
    return exitcodes


def generate_exitcode(exitcodes):
    print("Exit codes: %s" % exitcodes)
    exitcodes = set(exitcodes)
    if EXIT_SIGXCPU in exitcodes:
        exitcodes.remove(EXIT_SIGXCPU)
        exitcodes.add(EXIT_TIMEOUT)
    unexpected_codes = exitcodes - EXPECTED_EXITCODES
    if unexpected_codes:
        print("Error: Unexpected exit codes: %s" % list(unexpected_codes))
        if len(unexpected_codes) == 1:
            return unexpected_codes.pop()
        else:
            return EXIT_CRITICAL_ERROR
    for code in [EXIT_PLAN_FOUND, EXIT_UNSOLVABLE, EXIT_UNSOLVED_INCOMPLETE]:
        if code in exitcodes:
            return code
    for code in [EXIT_OUT_OF_MEMORY, EXIT_TIMEOUT]:
        if exitcodes == set([code]):
            return code
    if exitcodes == set([EXIT_OUT_OF_MEMORY, EXIT_TIMEOUT]):
        return EXIT_TIMEOUT_AND_MEMORY
    print("Error: Unhandled exit codes: %s" % exitcodes)
    return EXIT_CRITICAL_ERROR


def can_change_cost_type(args):
    return any('S_COST_TYPE' in part or 'H_COST_TYPE' in part for part in args)


def get_portfolio_attributes(portfolio):
    attributes = {}
    try:
        execfile(portfolio, attributes)
    except ImportError as err:
        if str(err) == "No module named portfolio":
            raise ValueError(
                "The portfolio format has changed. New portfolios may only "
                "define attributes. See the FDSS portfolios for examples.")
        else:
            raise
    if "CONFIGS" not in attributes:
        raise ValueError("portfolios must define CONFIGS")
    if "OPTIMAL" not in attributes:
        raise ValueError("portfolios must define OPTIMAL")
    return attributes


def run(portfolio, executable, sas_file):
    attributes = get_portfolio_attributes(portfolio)
    configs = attributes["CONFIGS"]
    optimal = attributes["OPTIMAL"]
    final_config = attributes.get("FINAL_CONFIG")
    final_config_builder = attributes.get("FINAL_CONFIG_BUILDER")
    timeout = attributes.get("TIMEOUT")
    # TODO: How should we retrieve the plan_file argument? We cannot
    #       add --plan_file to the main argument parser, since it has to
    #       be passed to the search component as well. Maybe we could
    #       use ArgumentParser.parse_known_args() to parse it here?
    plan_file = "sas_plan"
    # TODO: Remove or parse input_file with input_analyzer.is_unit_cost?
    #       If we remove this we will run the first satisficing FDSS
    #       configuration that finds a solution twice. This might
    #       be wasted effort, but the second run will use the lower
    #       g_bound. For all portfolios that do not use H_COST_TYPE and
    #       S_COST_TYPE, knowing whether the task has unit costs is
    #       irrelevant. I see no easy way of using the new --if-unit-cost
    #       for this problem.
    unitcost = "nonunit"

    # Time limits are either positive values in seconds or -1 (unlimited).
    soft_time_limit, hard_time_limit = resource.getrlimit(resource.RLIMIT_CPU)
    print("External time limits: %d, %d" % (soft_time_limit, hard_time_limit))
    external_time_limit = None
    if soft_time_limit != resource.RLIM_INFINITY:
        external_time_limit = soft_time_limit
    elif hard_time_limit != resource.RLIM_INFINITY:
        external_time_limit = hard_time_limit
    if (external_time_limit is not None and
            timeout is not None and
            timeout != external_time_limit):
        print("The externally set timeout (%d) differs from the one "
              "in the portfolio file (%d). Is this expected?" %
              (external_time_limit, timeout), file=sys.stderr)
    # Prefer limits in the order: external soft limit, external hard limit,
    # from portfolio file, default.
    if external_time_limit is not None:
        timeout = external_time_limit
    elif timeout is None:
        print("No timeout has been set for the portfolio so we take "
              "the default of %ds." % DEFAULT_TIMEOUT, file=sys.stderr)
        timeout = DEFAULT_TIMEOUT
    print("Internal time limit: %d" % timeout)

    # Memory limits are either positive values in Bytes or -1 (unlimited).
    soft_mem_limit, hard_mem_limit = resource.getrlimit(resource.RLIMIT_AS)
    print("External memory limits: %d, %d" % (soft_mem_limit, hard_mem_limit))
    # The soft memory limit is artificially lowered (by the downward script),
    # so we respect the hard limit and raise the soft limit for child processes.
    memory = hard_mem_limit - BYTES_FOR_PYTHON
    # Do not limit memory if the previous limit was very low or unlimited.
    if memory < 0:
        memory = None
    print("Internal memory limit: %s" % memory)

    remaining_time_at_start = float(timeout) - get_elapsed_time()
    print("remaining time at start: %.2f" % remaining_time_at_start)

    if optimal:
        exitcodes = run_opt(configs, executable, sas_file, plan_file,
                            remaining_time_at_start, memory)
    else:
        exitcodes = run_sat(configs, unitcost, executable, sas_file, plan_file,
                            final_config, final_config_builder,
                            remaining_time_at_start, memory)
    exitcode = generate_exitcode(exitcodes)
    if exitcode != 0:
        raise subprocess.CalledProcessError(exitcode, ["run-portfolio", portfolio])
