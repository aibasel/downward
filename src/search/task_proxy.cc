#include "task_proxy.h"

#include "causal_graph.h"

#include <iostream>

using namespace std;

void State::dump_pddl() const {
    for (FactProxy fact : (*this)) {
        const string &fact_name = fact.get_name();
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

const CausalGraph &TaskProxy::get_causal_graph() const {
    return ::get_causal_graph(task);
}
