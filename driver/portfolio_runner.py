# -*- coding: utf-8 -*-

from __future__ import print_function

""" Module for running planner portfolios.

Memory limits: We apply the same memory limit that is given to the
plan script to each planner call. Note that this setup does not work if
the sum of the memory usage of the Python process and the planner calls
is limited. In this case the Python process might get killed although
we would like to kill only the single planner call and continue with
the remaining configurations. If we ever want to support this scenario
we will have to reduce the memory limit of the planner calls by the
amount of memory that the Python process needs. On maia for example
this amounts to 128MB of reserved virtual memory. We can make Python
reserve less space by lowering the soft limit for virtual memory before
the process is started.
"""

__all__ = ["run"]

import os
import subprocess
import sys
import traceback

from . import call
from . import exitcodes
from . import limits
from . import util


DEFAULT_TIMEOUT = 1800


def adapt_args(args, search_cost_type, heuristic_cost_type, plan_manager):
    g_bound = plan_manager.get_best_plan_cost()
    plan_counter = plan_manager.get_plan_counter()
    print("g bound: %s" % g_bound)
    print("next plan number: %d" % (plan_counter + 1))

    for index, arg in enumerate(args):
        if arg == "--heuristic":
            heuristic = args[index + 1]
            heuristic = heuristic.replace("H_COST_TYPE", heuristic_cost_type)
            args[index + 1] = heuristic
        elif arg == "--search":
            search = args[index + 1]
            if "bound=BOUND" not in search:
                raise ValueError(
                    "Satisficing portfolios need the string "
                    "\"bound=BOUND\" in each search configuration. "
                    "See the FDSS portfolios for examples.")
            for name, value in [
                    ("BOUND", g_bound),
                    ("H_COST_TYPE", heuristic_cost_type),
                    ("S_COST_TYPE", search_cost_type)]:
                search = search.replace(name, str(value))
            args[index + 1] = search
            break


def run_search(executable, args, sas_file, plan_manager, time, memory):
    complete_args = [executable] + args + [
        "--internal-plan-file", plan_manager.get_plan_prefix()]
    print("args: %s" % complete_args)

    try:
        exitcode = call.check_call(
            complete_args, stdin=sas_file,
            time_limit=time, memory_limit=memory)
    except subprocess.CalledProcessError as err:
        exitcode = err.returncode
    print("exitcode: %d" % exitcode)
    print()
    return exitcode


def compute_run_time(timeout, configs, pos):
    remaining_time = timeout - util.get_elapsed_time()
    print("remaining time: {}".format(remaining_time))
    relative_time = configs[pos][0]
    remaining_relative_time = sum(config[0] for config in configs[pos:])
    print("config {}: relative time {}, remaining {}".format(
          pos, relative_time, remaining_relative_time))
    # For the last config we have relative_time == remaining_relative_time, so
    # we use all of the remaining time at the end.
    return remaining_time * relative_time / remaining_relative_time


def run_sat_config(configs, pos, search_cost_type, heuristic_cost_type,
                   executable, sas_file, plan_manager, timeout, memory):
    run_time = compute_run_time(timeout, configs, pos)
    if run_time <= 0:
        return None
    _, args_template = configs[pos]
    args = list(args_template)
    adapt_args(args, search_cost_type, heuristic_cost_type, plan_manager)
    args.extend([
        "--internal-previous-portfolio-plans", str(plan_manager.get_plan_counter())])
    result = run_search(executable, args, sas_file, plan_manager, run_time, memory)
    plan_manager.process_new_plans()
    return result


def run_sat(configs, executable, sas_file, plan_manager, final_config,
            final_config_builder, timeout, memory):
    # If the configuration contains S_COST_TYPE or H_COST_TYPE and the task
    # has non-unit costs, we start by treating all costs as one. When we find
    # a solution, we rerun the successful config with real costs.
    heuristic_cost_type = "one"
    search_cost_type = "one"
    changed_cost_types = False
    while configs:
        configs_next_round = []
        for pos, (relative_time, args) in enumerate(configs):
            exitcode = run_sat_config(
                configs, pos, search_cost_type, heuristic_cost_type,
                executable, sas_file, plan_manager, timeout, memory)
            if exitcode is None:
                return

            yield exitcode
            if exitcode == exitcodes.EXIT_UNSOLVABLE:
                return

            if exitcode == exitcodes.EXIT_PLAN_FOUND:
                configs_next_round.append(args)
                if (not changed_cost_types and can_change_cost_type(args) and
                    plan_manager.get_problem_type() == "general cost"):
                    print("Switch to real costs and repeat last run.")
                    changed_cost_types = True
                    search_cost_type = "normal"
                    heuristic_cost_type = "plusone"
                    exitcode = run_sat_config(
                        configs, pos, search_cost_type, heuristic_cost_type,
                        executable, sas_file, plan_manager, timeout, memory)
                    if exitcode is None:
                        return

                    yield exitcode
                    if exitcode == exitcodes.EXIT_UNSOLVABLE:
                        return
                if final_config_builder:
                    print("Build final config.")
                    final_config = final_config_builder(args)
                    break

        if final_config:
            break

        # Only run the successful configs in the next round.
        configs = configs_next_round

    if final_config:
        print("Abort portfolio and run final config.")
        exitcode = run_sat_config(
            [(1, final_config)], 0, search_cost_type,
            heuristic_cost_type, executable, sas_file, plan_manager,
            timeout, memory)
        if exitcode is not None:
            yield exitcode


def run_opt(configs, executable, sas_file, plan_manager, timeout, memory):
    for pos, (relative_time, args) in enumerate(configs):
        run_time = compute_run_time(timeout, configs, pos)
        exitcode = run_search(executable, args, sas_file, plan_manager,
                              run_time, memory)
        yield exitcode

        if exitcode in [exitcodes.EXIT_PLAN_FOUND, exitcodes.EXIT_UNSOLVABLE]:
            break


def can_change_cost_type(args):
    return any("S_COST_TYPE" in part or "H_COST_TYPE" in part for part in args)


def get_portfolio_attributes(portfolio):
    attributes = {}
    with open(portfolio) as portfolio_file:
        content = portfolio_file.read()
        try:
            exec(content, attributes)
        except Exception:
            traceback.print_exc()
            raise ImportError(
                "The portfolio %s could not be loaded. Maybe it still "
                "uses the old portfolio syntax? See the FDSS portfolios "
                "for examples using the new syntax." % portfolio)
    if "CONFIGS" not in attributes:
        raise ValueError("portfolios must define CONFIGS")
    if "OPTIMAL" not in attributes:
        raise ValueError("portfolios must define OPTIMAL")
    return attributes


def run(portfolio, executable, sas_file, plan_manager, time, memory):
    """
    Run the configs in the given portfolio file.

    The portfolio is allowed to run for at most *time* seconds and may
    use a maximum of *memory* bytes.
    """
    attributes = get_portfolio_attributes(portfolio)
    configs = attributes["CONFIGS"]
    optimal = attributes["OPTIMAL"]
    final_config = attributes.get("FINAL_CONFIG")
    final_config_builder = attributes.get("FINAL_CONFIG_BUILDER")
    if "TIMEOUT" in attributes:
        sys.exit(
            "The TIMEOUT attribute in portfolios has been removed. "
            "Please pass a time limit to fast-downward.py.")


    if time is None:
        if os.name == "nt":
            sys.exit(limits.RESOURCE_MODULE_MISSING_MSG)
        else:
            sys.exit(
                "Portfolios need a time limit. Please pass --search-time-limit "
                "or --overall-time-limit to fast-downward.py.")

    timeout = util.get_elapsed_time() + time

    if optimal:
        returncodes = run_opt(
            configs, executable, sas_file, plan_manager, timeout, memory)
    else:
        returncodes = run_sat(
            configs, executable, sas_file, plan_manager, final_config,
            final_config_builder, timeout, memory)
    exitcode = exitcodes.generate_exitcode(returncodes)
    if exitcode != 0:
        raise subprocess.CalledProcessError(exitcode, ["run-portfolio", portfolio])
