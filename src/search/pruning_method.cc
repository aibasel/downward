#include "pruning_method.h"

#include "plugin.h"

#include "task_utils/task_properties.h"
#include "utils/logging.h"

#include <cassert>

using namespace std;

PruningMethod::PruningMethod()
    : task(nullptr) {
}

void PruningMethod::initialize(const shared_ptr<AbstractTask> &task_) {
    assert(!task);
    task = task_;
    num_successors_before_pruning = 0;
    num_successors_after_pruning = 0;
}

void PruningMethod::prune_operators(
    const State &state, vector<OperatorID> &op_ids) {
    assert(!task_properties::is_goal_state(TaskProxy(*task), state));
    timer.resume();
    int num_ops_before_pruning = op_ids.size();
    prune(state, op_ids);
    num_successors_before_pruning += num_ops_before_pruning;
    num_successors_after_pruning += op_ids.size();
    timer.stop();
}

void PruningMethod::print_statistics() const {
    utils::g_log << "total successors before pruning: "
                 << num_successors_before_pruning << endl
                 << "total successors after pruning: "
                 << num_successors_after_pruning << endl;
    double pruning_ratio = (num_successors_before_pruning == 0) ? 1. : 1. - (
        static_cast<double>(num_successors_after_pruning) /
        static_cast<double>(num_successors_before_pruning));
    utils::g_log << "Pruning ratio: " << pruning_ratio << endl;
    utils::g_log << "Time for pruning operators: " << timer << endl;
}

static PluginTypePlugin<PruningMethod> _type_plugin(
    "PruningMethod",
    "Prune or reorder applicable operators.");
