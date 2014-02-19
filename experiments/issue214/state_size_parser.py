#! /usr/bin/env python

from lab.parser import Parser


def get_state_var_t_size():
    max_domain = 0
    offset = 0
    for line in open ('output'):
        line = line.strip()
        if line == 'begin_state':
            if (max_domain <= 255):
                return 1
            elif (max_domain <= 65535):
                return 2
            else:
                return 4
        if line == 'begin_variable':
            offset = 100
        offset += 1
        if offset == 104 and int(line) > max_domain:
          max_domain = int(line)

def calculate_old_state_size(content, props):
    if 'variables' in props:
        props['bytes_per_state_old'] = props['variables'] * get_state_var_t_size()

def measure_saved_bytes(content, props):
    if 'bytes_per_state_old' in props and 'bytes_per_state' in props:
        props['bytes_per_state_saved'] = props['bytes_per_state_old'] - props['bytes_per_state']

class StateSizeParser(Parser):
    def __init__(self):
        Parser.__init__(self)
        self.add_pattern('bytes_per_state', 'Bytes per state: (\d+)',
                        required=False, type=int)
        self.add_pattern('variables', 'Variables: (\d+)',
                        required=False, type=int)
        self.add_function(calculate_old_state_size)
        self.add_function(measure_saved_bytes)

if __name__ == '__main__':
    parser = StateSizeParser()
    print 'Running state size parser'
    parser.parse()
