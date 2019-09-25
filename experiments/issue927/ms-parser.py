#! /usr/bin/env python

import math
import re

from lab.parser import Parser

parser = Parser()
parser.add_pattern('ms_construction_time', 'Merge-and-shrink algorithm runtime: (.+)s', required=False, type=float)
parser.add_pattern('ms_atomic_construction_time', 'M&S algorithm timer: (.+)s \(after computation of atomic factors\)', required=False, type=float)
parser.add_pattern('ms_memory_delta', 'Final peak memory increase of merge-and-shrink algorithm: (\d+) KB', required=False, type=int)
parser.add_pattern('ms_num_remaining_factors', 'Number of remaining factors: (\d+)', required=False, type=int)
parser.add_pattern('ms_num_factors_kept', 'Number of factors kept: (\d+)', required=False, type=int)

def check_ms_constructed(content, props):
    ms_construction_time = props.get('ms_construction_time')
    abstraction_constructed = False
    if ms_construction_time is not None:
        abstraction_constructed = True
    props['ms_abstraction_constructed'] = abstraction_constructed

parser.add_function(check_ms_constructed)

def check_atomic_fts_constructed(content, props):
    ms_atomic_construction_time = props.get('ms_atomic_construction_time')
    ms_atomic_fts_constructed = False
    if ms_atomic_construction_time is not None:
        ms_atomic_fts_constructed = True
    props['ms_atomic_fts_constructed'] = ms_atomic_fts_constructed

parser.add_function(check_atomic_fts_constructed)

def check_planner_exit_reason(content, props):
    ms_abstraction_constructed = props.get('ms_abstraction_constructed')
    error = props.get('error')
    if error != 'success' and error != 'search-out-of-time' and error != 'search-out-of-memory':
        print('error: %s' % error)
        return

    # Check whether merge-and-shrink computation or search ran out of
    # time or memory.
    ms_out_of_time = False
    ms_out_of_memory = False
    search_out_of_time = False
    search_out_of_memory = False
    if ms_abstraction_constructed == False:
        if error == 'search-out-of-time':
            ms_out_of_time = True
        elif error == 'search-out-of-memory':
            ms_out_of_memory = True
    elif ms_abstraction_constructed == True:
        if error == 'search-out-of-time':
            search_out_of_time = True
        elif error == 'search-out-of-memory':
            search_out_of_memory = True
    props['ms_out_of_time'] = ms_out_of_time
    props['ms_out_of_memory'] = ms_out_of_memory
    props['search_out_of_time'] = search_out_of_time
    props['search_out_of_memory'] = search_out_of_memory

parser.add_function(check_planner_exit_reason)

def check_perfect_heuristic(content, props):
    plan_length = props.get('plan_length')
    expansions = props.get('expansions')
    if plan_length != None:
        perfect_heuristic = False
        if plan_length + 1 == expansions:
            perfect_heuristic = True
        props['perfect_heuristic'] = perfect_heuristic

parser.add_function(check_perfect_heuristic)

def add_construction_time_score(content, props):
    """
    Convert some properties into scores in the range [0, 1].

    Best possible performance in a task is counted as 1, while failure
    to solve a task and worst performance are counted as 0.

    """
    def log_score(value, min_bound, max_bound):
        if value is None or not props['ms_abstraction_constructed']:
            return 0
        value = max(value, min_bound)
        value = min(value, max_bound)
        raw_score = math.log(value) - math.log(max_bound)
        best_raw_score = math.log(min_bound) - math.log(max_bound)
        return raw_score / best_raw_score

    main_loop_max_time = None
    for line in content.splitlines():
        if line.startswith("Starting main loop"):
            if line.endswith("without a time limit."):
                try:
                    max_time = props['limit_search_time']
                except KeyError:
                    print("search time limit missing -> can't compute time scores")
                else:
                    main_loop_max_time = max_time
            else:
                z = re.match('Starting main loop with a time limit of (.+)s', line)
                assert(len(z.groups()) == 1)
                main_loop_max_time = float(z.groups()[0])
            break
    props['score_ms_construction_time'] = log_score(props.get('ms_construction_time'), min_bound=1.0, max_bound=main_loop_max_time)

parser.add_function(add_construction_time_score)

parser.parse()
