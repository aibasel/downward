#! /usr/bin/env python

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
    if error != 'success' and error != 'timeout' and error != 'out-of-memory':
        print 'error: %s' % error
        return

    # Check whether merge-and-shrink computation or search ran out of
    # time or memory.
    ms_out_of_time = False
    ms_out_of_memory = False
    search_out_of_time = False
    search_out_of_memory = False
    if ms_abstraction_constructed == False:
        if error == 'timeout':
            ms_out_of_time = True
        elif error == 'out-of-memory':
            ms_out_of_memory = True
    elif ms_abstraction_constructed == True:
        if error == 'timeout':
            search_out_of_time = True
        elif error == 'out-of-memory':
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

parser.parse()
