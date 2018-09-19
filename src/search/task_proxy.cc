#include "task_proxy.h"

#include "task_utils/causal_graph.h"

#include <iostream>

using namespace std;

const causal_graph::CausalGraph &TaskProxy::get_causal_graph() const {
    return causal_graph::get_causal_graph(task);
}
