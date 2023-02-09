#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <fstream>

#include "command_line.h"
#include "heuristic.h"
#include "search_engine.h"
#include "task_proxy.h"

#include "search_engines/enforced_hill_climbing_search.h"
#include "heuristics/ff_heuristic.h"
#include "tasks/root_task.h"
#include "task_utils/task_properties.h"
#include "utils/logging.h"
#include "utils/system.h"
#include "utils/timer.h"

namespace py = pybind11;

//py::object test(const std::string &sas_file, const std::string &cmd_line) {
void read_task(const std::string &sas_file) {
  std::ifstream task_file(sas_file);
  tasks::read_root_task(task_file);
}

class EvaluatorTrampoline : public Evaluator {
public:
    using Evaluator::Evaluator;
};

class HeuristicTrampoline : public Heuristic {
public:
    using Heuristic::Heuristic;

    virtual int compute_heuristic(const State &ancestor_state) override {
        PYBIND11_OVERRIDE_PURE(
            int,                /* Return type */
            Heuristic,          /* Parent class */
            compute_heuristic,  /* Name of function in C++ (must match Python name) */
            ancestor_state      /* Argument(s) */
        );
    }
};

PYBIND11_MODULE(pydownward, m) {
    m.doc() = "Gabi's pybind11 example plugin"; // Optional module docstring
                                                //
    m.def("read_task", &read_task, "Read the task from sas_file", py::arg("sas_file")="output.sas");

    m.def("get_root_task", &tasks::get_root_task, "Get the root task");

    py::class_<AbstractTask, std::shared_ptr<AbstractTask>>(m, "AbstractTask")
      .def("get_operator_name", &AbstractTask::get_operator_name);

    py::class_<OperatorID>(m, "OperatorID")
      .def("get_index", &OperatorID::get_index);

    py::class_<FactProxy>(m, "FactProxy")
      .def("get_name", &FactProxy::get_name);

    py::class_<State>(m, "State")
      .def("__getitem__", [](State &self, unsigned index)
        { return self[index]; })
      .def("__iter__", [](const State &s) { return py::make_iterator(begin(s), end(s)); }, py::keep_alive<0, 1>());

    py::class_<Evaluator, std::shared_ptr<Evaluator>, EvaluatorTrampoline>(m, "Evaluator");
  
    py::class_<Heuristic, std::shared_ptr<Heuristic>, Evaluator, HeuristicTrampoline>(m, "Heuristic")
      .def(py::init<std::shared_ptr<AbstractTask>>())
      .def("compute_heuristic", &Heuristic::compute_heuristic);

    py::class_<ff_heuristic::FFHeuristic, std::shared_ptr<ff_heuristic::FFHeuristic>, Heuristic>(m, "FFHeuristic")
      .def(py::init<std::shared_ptr<AbstractTask>>());

    py::class_<enforced_hill_climbing_search::EnforcedHillClimbingSearch, std::shared_ptr<enforced_hill_climbing_search::EnforcedHillClimbingSearch>>(m, "EHCSearch")
      .def(py::init<const std::string &, int, double, int, std::shared_ptr<Evaluator>>())
      .def("search", &enforced_hill_climbing_search::EnforcedHillClimbingSearch::search)
      .def("found_solution", &enforced_hill_climbing_search::EnforcedHillClimbingSearch::found_solution)
      .def("get_plan", &enforced_hill_climbing_search::EnforcedHillClimbingSearch::get_plan);
}
