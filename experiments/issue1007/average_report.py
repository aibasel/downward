# -*- coding: utf-8 -*-

from downward.reports import PlanningReport
from lab import tools
from lab.reports import geometric_mean

import os

DEBUG=False

class AverageAlgorithmReport(PlanningReport):
    """
    This currently only works for some hard-coded attributes.
    """
    def __init__(self, algo_name_suffixes, **kwargs):
        PlanningReport.__init__(self, **kwargs)
        self.algo_name_suffixes=algo_name_suffixes

    def get_text(self):
        if not self.outfile.endswith("properties"):
            raise ValueError("outfile must be a path to a properties file")
        algo_infixes = set()
        for algo in self.algorithms:
            for suffix in self.algo_name_suffixes:
                if suffix in algo:
                    algo_infixes.add(algo.replace(suffix, ''))
                    break
        # print(algo_infixes)
        # print(self.algo_name_suffixes)
        props = tools.Properties(self.outfile)
        for domain, problem in self.problem_runs.keys():
            if DEBUG:
                print(domain, problem)
            for algo in algo_infixes:
                if DEBUG:
                    print("Consider ", algo)
                properties_key = algo + '-' + domain + '-' + problem
                average_algo_dict = {}
                average_algo_dict['algorithm'] = algo
                average_algo_dict['domain'] = domain
                average_algo_dict['problem'] = problem
                average_algo_dict['id'] = [algo, domain, problem]
                for attribute in self.attributes:
                    if DEBUG:
                        print("Consider ", attribute)
                    values = []
                    for suffix in self.algo_name_suffixes:
                        real_algo = algo + suffix
                        # if DEBUG:
                            # print("Composed algo ", real_algo)
                        real_algo_run = self.runs[(domain, problem, real_algo)]
                        values.append(real_algo_run.get(attribute))
                    if DEBUG:
                        print(values)
                    values_without_none = [value for value in values if value is not None]
                    if attribute in [
                            'coverage', 'cegar_num_iterations',
                            'cegar_num_patterns',
                            'cegar_total_pdb_size', 'initial_h_value'
                            'coverage', 'initial_h_value',
                            'cpdbs_num_patterns', 'cpdbs_total_pdb_size',
                            'cegar_num_iterations', 'cegar_num_patterns',
                            'cegar_total_pdb_size',
                        ] or 'score' in attribute:
                        # if 'score' not in attribute:
                            # assert len(values_without_none) == 10 # does not hold for scores
                        average_value = sum(values_without_none)/float(len(values))
                    elif 'time' in attribute or 'expansions' in attribute:
                        if len(values_without_none) == 10:
                            average_value = geometric_mean(values_without_none)
                        else:
                            average_value = None
                    else:
                        print("Don't know how to handle {}".format(attribute))
                        exit(1)
                    average_algo_dict[attribute] = average_value
                props[properties_key] = average_algo_dict
        return str(props)
