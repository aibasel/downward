#include "pruning_method.h"

#include "global_operator.h" // TODO: Remove once issue629 is merged.
#include "globals.h" // TODO: Remove once issue629 is merged.
#include "plugin.h"

using namespace std;

static PluginTypePlugin<PruningMethod> _type_plugin(
    "PruningMethod",
    "Prune or reorder applicable operators.");


void PruningMethod::prune_operators(const GlobalState &state, vector<int> &op_ids) {
    vector<const GlobalOperator *> operators;
    operators.reserve(op_ids.size());
    for (int op_id : op_ids) {
        operators.push_back(&g_operators[op_id]);
    }

    prune_operators(state, operators);

    if (operators.size() < op_ids.size()) {
        vector<int> pruned_op_ids;
        for (const GlobalOperator *op : operators) {
            pruned_op_ids.push_back(get_op_index_hacked(op));
        }
        op_ids.swap(pruned_op_ids);
    }
}
