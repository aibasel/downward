#! /usr/bin/env python

from lab.parser import Parser

parser = Parser()
parser.add_pattern('ms_final_size', 'Final transition system size: (\d+)', required=False, type=int)
parser.add_pattern('ms_construction_time', 'Done initializing merge-and-shrink heuristic \[(.+)s\]', required=False, type=float)
parser.add_pattern('ms_memory_delta', 'Final peak memory increase of merge-and-shrink computation: (\d+) KB', required=False, type=int)
parser.add_pattern('actual_search_time', 'Actual search time: (.+)s \[t=.+s\]', required=False, type=float)

def check_ms_constructed(content, props):
    ms_construction_time = props.get('ms_construction_time')
    abstraction_constructed = False
    if ms_construction_time is not None:
        abstraction_constructed = True
    props['ms_abstraction_constructed'] = abstraction_constructed

parser.add_function(check_ms_constructed)

def check_planner_exit_reason(content, props):
    ms_abstraction_constructed = props.get('ms_abstraction_constructed')
    error = props.get('error')
    if error != 'none' and error != 'timeout' and error != 'out-of-memory':
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

def check_proved_unsolvability(content, props):
    proved_unsolvability = False
    if props['coverage'] == 0:
        for line in content.splitlines():
            if line == 'Completely explored state space -- no solution!':
                proved_unsolvability = True
                break
    props['proved_unsolvability'] = proved_unsolvability

parser.add_function(check_proved_unsolvability)

def count_dfp_no_goal_relevant_ts(content, props):
    counter = 0
    for line in content.splitlines():
        if line == 'found no goal relevant pair':
            counter += 1
    props['ms_dfp_nogoalrelevantpair_counter'] = counter

parser.add_function(count_dfp_no_goal_relevant_ts)

parser.parse()
