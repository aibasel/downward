#include "util.h"

#include "landmark.h"
#include "landmark_graph.h"

#include "../task_proxy.h"
#include "../utils/logging.h"

#include <limits>

using namespace std;

namespace landmarks {
bool _possibly_fires(const EffectConditionsProxy &conditions, const vector<vector<int>> &lvl_var) {
    for (FactProxy cond : conditions)
        if (lvl_var[cond.get_variable().get_id()][cond.get_value()] == numeric_limits<int>::max())
            return false;
    return true;
}

unordered_map<int, int> _intersect(const unordered_map<int, int> &a, const unordered_map<int, int> &b) {
    if (a.size() > b.size())
        return _intersect(b, a);
    unordered_map<int, int> result;
    for (const auto &pair_a : a) {
        const auto it_b = b.find(pair_a.first);
        if (it_b != b.end() && it_b->second == pair_a.second)
            result.insert(pair_a);
    }
    return result;
}

bool possibly_reaches_lm(const OperatorProxy &op,
                         const vector<vector<int>> &lvl_var,
                         const Landmark &landmark) {
    /* Check whether operator o can possibly make landmark lmp true in a
       relaxed task (as given by the reachability information in lvl_var) */

    assert(!lvl_var.empty());

    // Test whether all preconditions of o can be reached
    // Otherwise, operator is not applicable
    PreconditionsProxy preconditions = op.get_preconditions();
    for (FactProxy pre : preconditions)
        if (lvl_var[pre.get_variable().get_id()][pre.get_value()] ==
            numeric_limits<int>::max())
            return false;

    // Go through all effects of o and check whether one can reach a
    // proposition in lmp
    for (EffectProxy effect: op.get_effects()) {
        FactProxy effect_fact = effect.get_fact();
        assert(!lvl_var[effect_fact.get_variable().get_id()].empty());
        for (const FactPair &fact : landmark.facts) {
            if (effect_fact.get_pair() == fact) {
                if (_possibly_fires(effect.get_conditions(), lvl_var))
                    return true;
                break;
            }
        }
    }

    return false;
}

OperatorProxy get_operator_or_axiom(const TaskProxy &task_proxy, int op_or_axiom_id) {
    if (op_or_axiom_id < 0) {
        return task_proxy.get_axioms()[-op_or_axiom_id - 1];
    } else {
        return task_proxy.get_operators()[op_or_axiom_id];
    }
}

int get_operator_or_axiom_id(const OperatorProxy &op) {
    if (op.is_axiom()) {
        return -op.get_id() - 1;
    } else {
        return op.get_id();
    }
}

static void dump_node(const TaskProxy &task_proxy, const LandmarkNode &node) {
    cout << "  lm" << node.get_id() << " [label=\"";
    bool first = true;
    const Landmark &landmark = node.get_landmark();
    for (FactPair fact : landmark.facts) {
        if (!first) {
            if (landmark.disjunctive) {
                cout << " | ";
            } else if (landmark.conjunctive) {
                cout << " & ";
            }
        }
        first = false;
        VariableProxy var = task_proxy.get_variables()[fact.var];
        cout << var.get_fact(fact.value).get_name();
    }
    cout << "\"";
    if (landmark.is_true_in_state(task_proxy.get_initial_state())) {
        cout << ", style=bold";
    }
    if (landmark.is_true_in_goal) {
        cout << ", style=filled";
    }
    cout << "];\n";
}

static void dump_edge(int from, int to, EdgeType edge) {
    cout << "      lm" << from << " -> lm" << to << " [label=";
    switch (edge) {
    case EdgeType::NECESSARY:
        cout << "\"nec\"";
        break;
    case EdgeType::GREEDY_NECESSARY:
        cout << "\"gn\"";
        break;
    case EdgeType::NATURAL:
        cout << "\"n\"";
        break;
    case EdgeType::REASONABLE:
        cout << "\"r\", style=dashed";
        break;
    case EdgeType::OBEDIENT_REASONABLE:
        cout << "\"o_r\", style=dashed";
        break;
    }
    cout << "];\n";
}

void dump_landmark_graph(const TaskProxy &task_proxy, const LandmarkGraph &graph) {
    utils::g_log << "Dumping landmark graph: " << endl;

    cout << "digraph G {\n";
    for (const unique_ptr<LandmarkNode> &node : graph.get_nodes()) {
        dump_node(task_proxy, *node);
        for (const auto &child : node->children) {
            const LandmarkNode *child_node = child.first;
            const EdgeType &edge = child.second;
            dump_edge(node->get_id(), child_node->get_id(), edge);
        }
    }
    cout << "}" << endl;
    utils::g_log << "Landmark graph end." << endl;
}
}
