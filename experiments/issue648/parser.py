#! /usr/bin/env python

from lab.parser import Parser

parser = Parser()

def check_planner_exit_reason(content, props):
    error = props.get('error')
    if error != 'none' and error != 'timeout' and error != 'out-of-memory':
        print 'error: %s' % error
        return

    out_of_time = False
    out_of_memory = False
    if error == 'timeout':
        out_of_time = True
    elif error == 'out-of-memory':
        out_of_memory = True
    props['out_of_time'] = out_of_time
    props['out_of_memory'] = out_of_memory

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

parser.parse()
