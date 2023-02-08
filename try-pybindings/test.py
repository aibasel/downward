#! /usr/bin/env python

import pydownward

class MyHeuristic(pydownward.Heuristic):
    def __init__(self, task):
        super().__init__(task)
        self.ff = pydownward.FFHeuristic(task)

    def compute_heuristic(self, state):
        ff = self.ff.compute_heuristic(state)
        print(f"Returning h = {ff} for state: {[f.get_name() for f in state]}")
        return ff

pydownward.read_task("output.sas")
task = pydownward.get_root_task()

h = MyHeuristic(task)
search = pydownward.EHCSearch("hallo", 0, 100.0, 100, h) 
search.search()
assert search.found_solution()
op_id_plan = search.get_plan()
plan = [task.get_operator_name(op_id.get_index(), False)
        for op_id in op_id_plan]
print(plan)