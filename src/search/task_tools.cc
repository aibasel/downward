#include "task_tools.h"

#include "../utils/system.h"

#include <iostream>

using namespace std;
using utils::ExitCode;


bool is_unit_cost(const TaskProxy &task_proxy) {
    for (OperatorProxy op : task_proxy.get_operators()) {
        if (op.get_cost() != 1)
            return false;
    }
    return true;
}

bool has_axioms(const TaskProxy &task_proxy) {
    return !task_proxy.get_axioms().empty();
}

void verify_no_axioms(const TaskProxy &task_proxy) {
    if (has_axioms(task_proxy)) {
        cerr << "This configuration does not support axioms!"
             << endl << "Terminating." << endl;
        utils::exit_with(ExitCode::UNSUPPORTED);
    }
}


static int get_first_conditional_effects_op_id(const TaskProxy &task_proxy) {
    for (OperatorProxy op : task_proxy.get_operators()) {
        for (EffectProxy effect : op.get_effects()) {
            if (!effect.get_conditions().empty())
                return op.get_id();
        }
    }
    return -1;
}

bool has_conditional_effects(const TaskProxy &task_proxy) {
    return get_first_conditional_effects_op_id(task_proxy) != -1;
}

void verify_no_conditional_effects(const TaskProxy &task_proxy) {
    int op_id = get_first_conditional_effects_op_id(task_proxy);
    if (op_id != -1) {
        OperatorProxy op = task_proxy.get_operators()[op_id];
        cerr << "This configuration does not support conditional effects "
             << "(operator " << op.get_name() << ")!" << endl
             << "Terminating." << endl;
        utils::exit_with(ExitCode::UNSUPPORTED);
    }
}

double get_average_operator_cost(const TaskProxy &task_proxy) {
    double average_operator_cost = 0;
    for (OperatorProxy op : task_proxy.get_operators()) {
        average_operator_cost += op.get_cost();
    }
    average_operator_cost /= task_proxy.get_operators().size();
    return average_operator_cost;
}
