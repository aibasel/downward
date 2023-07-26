To install pybind11, use
  apt install python3-pybind11

To test the current pybind setup, proceed as follows:

$ mkdir pybind-build
$ cd pybind-build
$ cmake .. -DPYTHON_LIBRARY_DIR="<absolutepathtorepositoryroot>/try-pybindings" -DPYTHON_EXECUTABLE="/usr/bin/python3"
 (or wherever your python3 lives)
$ make -j8
$ make install
$ cd ../try-pybindings
$ ls
pydownward.cpython-310-x86_64-linux-gnu.so  test.py
$ cat test.py

Put some output.sas file into the directory. The test only runs enforced
hill-climbing, which has a high likelihood to NOT solve a task. It can for
example solve the first driverlog task, so if in doubt, use that one.

Enjoy:
$ python -i test.py


Notes on the interface:

* We hacked the code in some places
  - We made Heuristic::compute_heuristic public. This might be necessary for the
    trampoline classes but have not looked for alternatives.
  - We added a constructor Heuristic(task) that fixes some options (see
    create_dummy_options_for_python_binding in heuristic.cc). The long term plan
    would be t not pass the options object into the constructor and instead pass
    individual parameters that are then also all exposed through the interface.
  - Similarly, we added temporary constructors for the FF heuristic, search
    engines, and EHC search.
  - We added a global function get_root_task to access the global task. We have
    to think about how to expose the task.

* The order of template parameters in py::class_<...> does not matter. Pybind
  will figure out which parameter is which. We should agree on some default order.
  For Heuristic, we use four parameters:
  - Heuristic is the class to expose to Python
  - std::shared_ptr<Heuristic> is the holder that is internally used for
    reference counting, we use shared pointers to make this compatible with our
    other features.
  - Evaluator is the base class
  - HeuristicTrampoline is a class used to handle function calls of virtual
    functions. If we inherit from Heuristic in Python, the trampoline class is
    responsible for forwarding calls to the Python class. See
    https://pybind11.readthedocs.io/en/stable/advanced/classes.html

* Trampoline classes (EvaluatorTrampoline, HeuristicTrampoline) are needed to
  derive classes in Python. They are needed for all classes in the hierarchy. For
  example, if we wanted to allow users to inherit from FFHeuristic, it would also
  need a trampoline class. They have to define all virtual methods that we want to
  make available in Python on every level (FFHeuristicTrampoline would have to
  repeat the definition of compute_heuristic). It is not clear if the trampoline
  for Evaluator is needed as long as no virtual method from Evaluator is in the
  interface. Probably yes, because the constructor counts.

* py::make_iterator can be used to set up an iterable but py::keep_alive is
  required to keep the iterator alive while it is used.
