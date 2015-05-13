#include "task_tools.h"

#include "utilities.h"

#include <iostream>

using namespace std;

bool is_unit_cost(TaskProxy task) {
    for (OperatorProxy op : task.get_operators()) {
        if (op.get_cost() != 1)
            return false;
    }
    return true;
}

bool has_axioms(TaskProxy task) {
    return !task.get_axioms().empty();
}

void verify_no_axioms(TaskProxy task) {
    if (has_axioms(task)) {
        cerr << "This configuration does not support axioms!"
             << endl << "Terminating." << endl;
        exit_with(EXIT_UNSUPPORTED);
    }
}


static int get_first_conditional_effects_op_id(TaskProxy task) {
    for (OperatorProxy op : task.get_operators()) {
        for (EffectProxy effect : op.get_effects()) {
            if (!effect.get_conditions().empty())
                return op.get_id();
        }
    }
    return -1;
}

bool has_conditional_effects(TaskProxy task) {
    return get_first_conditional_effects_op_id(task) != -1;
}

void verify_no_conditional_effects(TaskProxy task) {
    int op_id = get_first_conditional_effects_op_id(task);
    if (op_id != -1) {
        OperatorProxy op = task.get_operators()[op_id];
        cerr << "This configuration does not support conditional effects "
             << "(operator " << op.get_name() << ")!" << endl
             << "Terminating." << endl;
        exit_with(EXIT_UNSUPPORTED);
    }
}

void dump_fact(FactProxy fact) {
    cout << "  " << fact.get_variable().get_id() << "=" << fact.get_value()
         << ": " << fact.get_name() << endl;
}

void dump_variable(VariableProxy var) {
    cout << "Variable " << var.get_id() << " (" << var.get_name() << ", "
         << var.get_domain_size() << " values):" << endl;
    for (int value = 0; value < var.get_domain_size(); ++value) {
        FactProxy fact = var.get_fact(value);
        dump_fact(fact);
    }
}

void dump_state(const State &state) {
    for (FactProxy fact : state) {
        dump_fact(fact);
    }
}

void dump_operator(OperatorProxy op) {
    cout << "Operator " << op.get_id() << " (" << op.get_name()
         << ", cost " << op.get_cost() << ")" << endl;
    // TODO: Dump preconditions and effects.
}

void dump_task(TaskProxy task) {
    for (VariableProxy var : task.get_variables())
        dump_variable(var);
    cout << "Initial state:" << endl;
    dump_state(task.get_initial_state());
    cout << "Goals:" << endl;
    for (FactProxy goal : task.get_goals()) {
        dump_fact(goal);
    }
    for (OperatorProxy op : task.get_operators())
        dump_operator(op);
}
