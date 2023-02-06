import pydownward
pydownward.read_task("output.sas")
task = pydownward.get_root_task()
ff = pydownward.FFHeuristic(task)
search = pydownward.EHCSearch("hallo", 0, 100.0, 100, ff) 
search.search()
assert search.found_solution()
op_id_plan = search.get_plan()
plan= [task.get_operator_name(op_id.get_index(), False)
       for op_id in op_id_plan]
