#include "task_proxy.h"

#include "task_utils/causal_graph.h"

#include <iostream>

using namespace std;

void State::dump_pddl() const {
    for (FactProxy fact : (*this)) {
        string fact_name = fact.get_name();
        if (fact_name != "<none of those>")
            cout << fact_name << endl;
    }
}

void State::dump_fdr() const {
    for (FactProxy fact : (*this)) {
        VariableProxy var = fact.get_variable();
        cout << "  #" << var.get_id() << " [" << var.get_name() << "] -> "
             << fact.get_value() << endl;
    }
}

void GoalsProxy::dump() const {
    cout << "Goal conditions:" << endl;
    for (FactProxy goal : (*this)) {
        cout << "  " << goal.get_variable().get_name() << ": "
             << goal.get_value() << endl;
    }
}

void TaskProxy::dump() const {
    OperatorsProxy operators = get_operators();
    int min_action_cost = numeric_limits<int>::max();
    int max_action_cost = 0;
    for (OperatorProxy op : operators) {
        min_action_cost = min(min_action_cost, op.get_cost());
        max_action_cost = max(max_action_cost, op.get_cost());
    }
    cout << "Min Action Cost: " << min_action_cost << endl;
    cout << "Max Action Cost: " << max_action_cost << endl;

    VariablesProxy variables = get_variables();
    cout << "Variables (" << variables.size() << "):" << endl;
    for (VariableProxy var : variables) {
        cout << "  " << var.get_name()
             << " (range " << var.get_domain_size() << ")" << endl;
        for (int val = 0; val < var.get_domain_size(); ++val) {
            cout << "    " << val << ": " << var.get_fact(val).get_name() << endl;
        }
    }
    State initial_state = get_initial_state();
    cout << "Initial State (PDDL):" << endl;
    initial_state.dump_pddl();
    cout << "Initial State (FDR):" << endl;
    initial_state.dump_fdr();
    get_goals().dump();
}

const causal_graph::CausalGraph &TaskProxy::get_causal_graph() const {
    return causal_graph::get_causal_graph(task);
}
