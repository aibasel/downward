#include "abstract_task.h"

#include "per_task_information.h"
#include "plugin.h"

#include <iostream>

using namespace std;

const FactPair FactPair::no_fact = FactPair(-1, -1);

ostream &operator<<(ostream &os, const FactPair &fact_pair) {
    os << fact_pair.var << "=" << fact_pair.value;
    return os;
}

bool has_conditional_effects(const AbstractTask &task) {
    int num_ops = task.get_num_operators();
    for (int op_index = 0; op_index < num_ops; ++op_index) {
        int num_effs = task.get_num_operator_effects(op_index, false);
        for (int eff_index = 0; eff_index < num_effs; ++eff_index) {
            int num_conditions = task.get_num_operator_effect_conditions(
                    op_index, eff_index, false);
            if (num_conditions > 0) {
                return true;
            }
        }
    }
    return false;
}

static PluginTypePlugin<AbstractTask> _type_plugin(
    "AbstractTask",
    // TODO: Replace empty string by synopsis for the wiki page.
    "");
