#include "task_proxy.h"

#include "causal_graph.h"


const CausalGraph &TaskProxy::get_causal_graph() const {
    return ::get_causal_graph(task);
}
