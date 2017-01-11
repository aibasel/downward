#include "pruning_method.h"

#include "global_state.h"
#include "plugin.h"

#include <cassert>

using namespace std;

PruningMethod::PruningMethod()
    : task(nullptr) {
}

void PruningMethod::initialize(const shared_ptr<AbstractTask> &task_) {
    assert(!task);
    task = task_;
}

// TODO remove this overload once the search uses the task interface.
void PruningMethod::prune_operators(const GlobalState &global_state,
                                    vector<OperatorID> &op_ids) {
    assert(task);
    /* Note that if the pruning method would use a different task than
       the search, we would have to convert the state before using it. */
    State state(*task, global_state.get_values());

    prune_operators(state, op_ids);
}

static PluginTypePlugin<PruningMethod> _type_plugin(
    "PruningMethod",
    "Prune or reorder applicable operators.");
