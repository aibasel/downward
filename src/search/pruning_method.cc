#include "pruning_method.h"

#include "global_operator.h"
#include "globals.h"
#include "plugin.h"
#include "utils/collections.h"

#include <algorithm>

using namespace std;

/* TODO: get_op_index belongs to a central place.
   We currently have copies of it in different parts of the code. */
static inline int get_op_index(const GlobalOperator *op) {
    int op_index = op - &*g_operators.begin();
    assert(op_index >= 0 && op_index < static_cast<int>(g_operators.size()));
    return op_index;
}

/*
  TODO: once we switch the search to the task interface, we have to think
  about a better way to get the task into the pruning method. Possible options
  are to use the constructor (as in Heuristic), to use an initialize method
  (e.g., MergeStrategy), or to use a factory (e.g., PatternGenerator)
*/
PruningMethod::PruningMethod()
    : task(g_root_task()),
      task_proxy(*task) {
}

// TODO remove this overload once the search uses the task interface.
void PruningMethod::prune_operators(const GlobalState &global_state,
                                    vector<const GlobalOperator *> &global_ops) {
    /* Note that if the pruning method would use a different task than
       the search, we would have to convert the state before using it. */
    State state(*g_root_task(), global_state.get_values());

    OperatorsProxy operators = task_proxy.get_operators();
    vector<OperatorProxy> local_ops;
    local_ops.reserve(global_ops.size());
    for (const GlobalOperator *global_op : global_ops) {
        int op_id = get_op_index(global_op);
        local_ops.push_back(operators[op_id]);
    }

    prune_operators(state, local_ops);

    // If we pruned anything, translate the local operators back to global operators.
    if (local_ops.size() < global_ops.size()) {
        vector<const GlobalOperator *> pruned_ops;
        for (OperatorProxy op : local_ops) {
            pruned_ops.push_back(&g_operators[op.get_id()]);
        }

        global_ops.swap(pruned_ops);
        /* Note that we don't need to sort here, but it seems to help
           performance for some reason. */
        sort(global_ops.begin(), global_ops.end());
    }
}

static PluginTypePlugin<PruningMethod> _type_plugin(
    "PruningMethod",
    "Prune or reorder applicable operators.");
