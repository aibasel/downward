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
    num_unpruned_successors_generated = 0;
    num_pruned_successors_generated = 0;
}

void PruningMethod::prune_op_ids(
    const State &state, vector<OperatorID> &op_ids) {
    assert(!task_properties::is_goal_state(TaskProxy(*task), state));
    timer.resume();
    int num_ops_before_pruning = op_ids.size();
    prune_operators(state, op_ids);
    num_unpruned_successors_generated += num_ops_before_pruning;
    num_pruned_successors_generated += op_ids.size();
    timer.stop();
}

void PruningMethod::print_statistics() const {
    utils::g_log << "total successors before partial-order reduction: "
                 << num_unpruned_successors_generated << endl
                 << "total successors after partial-order reduction: "
                 << num_pruned_successors_generated << endl;
    double pruning_ratio = (num_unpruned_successors_generated == 0) ? 1. : 1. - (
        static_cast<double>(num_pruned_successors_generated) /
        static_cast<double>(num_unpruned_successors_generated));
    utils::g_log << "Pruning ratio: " << pruning_ratio << endl;
    utils::g_log << "Time for pruning operators: " << timer << endl;
}

static PluginTypePlugin<PruningMethod> _type_plugin(
    "PruningMethod",
    "Prune or reorder applicable operators.");
