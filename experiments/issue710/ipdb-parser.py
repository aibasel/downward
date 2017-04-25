#! /usr/bin/env python

from lab.parser import Parser

parser = Parser()
parser.add_pattern('hc_iterations', 'iPDB: iterations = (\d+)', required=False, type=int)
parser.add_pattern('hc_num_patters', 'iPDB: number of patterns = (\d+)', required=False, type=int)
parser.add_pattern('hc_size', 'iPDB: size = (\d+)', required=False, type=int)
parser.add_pattern('hc_num_generated', 'iPDB: generated = (\d+)', required=False, type=int)
parser.add_pattern('hc_num_rejected', 'iPDB: rejected = (\d+)', required=False, type=int)
parser.add_pattern('hc_max_pdb_size', 'iPDB: maximum pdb size = (\d+)', required=False, type=int)
parser.add_pattern('hc_hill_climbing_time', 'iPDB: hill climbing time: (.+)s', required=False, type=float)
parser.add_pattern('hc_total_time', 'Pattern generation \(hill climbing\) time: (.+)s', required=False, type=float)
parser.add_pattern('cpdbs_time', 'PDB collection construction time: (.+)s', required=False, type=float)

def check_hc_constructed(content, props):
    hc_time = props.get('hc_total_time')
    abstraction_constructed = False
    if hc_time is not None:
        abstraction_constructed = True
    props['hc_abstraction_constructed'] = abstraction_constructed

parser.add_function(check_hc_constructed)

def check_planner_exit_reason(content, props):
    hc_abstraction_constructed = props.get('hc_abstraction_constructed')
    error = props.get('error')
    if error != 'none' and error != 'timeout' and error != 'out-of-memory':
        print 'error: %s' % error
        return

    # Check whether hill climbing computation or search ran out of
    # time or memory.
    hc_out_of_time = False
    hc_out_of_memory = False
    search_out_of_time = False
    search_out_of_memory = False
    if hc_abstraction_constructed == False:
        if error == 'timeout':
            hc_out_of_time = True
        elif error == 'out-of-memory':
            hc_out_of_memory = True
    elif hc_abstraction_constructed == True:
        if error == 'timeout':
            search_out_of_time = True
        elif error == 'out-of-memory':
            search_out_of_memory = True
    props['hc_out_of_time'] = hc_out_of_time
    props['hc_out_of_memory'] = hc_out_of_memory
    props['search_out_of_time'] = search_out_of_time
    props['search_out_of_memory'] = search_out_of_memory

parser.add_function(check_planner_exit_reason)

parser.parse()
