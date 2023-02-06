#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <fstream>

#include "command_line.h"
#include "search_engine.h"

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

PYBIND11_MODULE(pydownward, m) {
    m.doc() = "Gabi's pybind11 example plugin"; // Optional module docstring
                                                //
    m.def("read_task", &read_task, "Read the task from sas_file", py::arg("sas_file")="output.sas");

    m.def("get_root_task", &tasks::get_root_task, "Get the root task");

    py::class_<AbstractTask, std::shared_ptr<AbstractTask>>(m, "AbstractTask")
      .def("get_operator_name", &AbstractTask::get_operator_name);

    py::class_<Evaluator, std::shared_ptr<Evaluator>>(m, "Evaluator");

    py::class_<OperatorID>(m, "OperatorID")
      .def("get_index", &OperatorID::get_index);

    py::class_<ff_heuristic::FFHeuristic, std::shared_ptr<ff_heuristic::FFHeuristic>, Evaluator>(m, "FFHeuristic")
      .def(py::init<std::shared_ptr<AbstractTask>>());

    py::class_<enforced_hill_climbing_search::EnforcedHillClimbingSearch, std::shared_ptr<enforced_hill_climbing_search::EnforcedHillClimbingSearch>>(m, "EHCSearch")
      .def(py::init<const std::string &, int, double, int, std::shared_ptr<Evaluator>>())
      .def("search", &enforced_hill_climbing_search::EnforcedHillClimbingSearch::search)
      .def("found_solution", &enforced_hill_climbing_search::EnforcedHillClimbingSearch::found_solution)
      .def("get_plan", &enforced_hill_climbing_search::EnforcedHillClimbingSearch::get_plan);
}
