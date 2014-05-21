#! /usr/bin/env python

from lab.parser import Parser

def calculate_old_state_size(content, props):
    if 'bytes_per_state' not in props and  'preprocessor_variables' in props and 'state_var_t_size' in props:
        props['bytes_per_state'] = props['preprocessor_variables'] * props['state_var_t_size']

class StateSizeParser(Parser):
    def __init__(self):
        Parser.__init__(self)
        self.add_pattern('bytes_per_state', 'Bytes per state: (\d+)',
                        required=False, type=int)
        self.add_pattern('state_var_t_size', 'Dispatcher selected state size (\d).',
                        required=False, type=int)
        self.add_pattern('variables', 'Variables: (\d+)',
                        required=False, type=int)
        self.add_function(calculate_old_state_size)

if __name__ == '__main__':
    parser = StateSizeParser()
    print 'Running state size parser'
    parser.parse()
