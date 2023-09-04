#include "util.h"

#include "landmark.h"
#include "landmark_graph.h"

#include "../task_proxy.h"
#include "../utils/logging.h"

#include <limits>

using namespace std;

namespace landmarks {
static bool _possibly_fires(const EffectConditionsProxy &conditions, const vector<vector<bool>> &reached) {
    for (FactProxy cond : conditions)
        if (!reached[cond.get_variable().get_id()][cond.get_value()])
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
                         const vector<vector<bool>> &reached,
                         const Landmark &landmark) {
    /* Check whether operator o can possibly make landmark lmp true in a
       relaxed task (as given by the reachability information in reached) */

    assert(!reached.empty());

    // Test whether all preconditions of o can be reached
    // Otherwise, operator is not applicable
    PreconditionsProxy preconditions = op.get_preconditions();
    for (FactProxy pre : preconditions)
        if (!reached[pre.get_variable().get_id()][pre.get_value()])
            return false;

    // Go through all effects of o and check whether one can reach a
    // proposition in lmp
    for (EffectProxy effect: op.get_effects()) {
        FactProxy effect_fact = effect.get_fact();
        assert(!reached[effect_fact.get_variable().get_id()].empty());
        for (const FactPair &fact : landmark.facts) {
            if (effect_fact.get_pair() == fact) {
                if (_possibly_fires(effect.get_conditions(), reached))
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

/*
  The below functions use cout on purpose for dumping a landmark graph.
  TODO: ideally, this should be written to a file or through a logger
  at least, but without the time and memory stamps.
*/
static void dump_node(
    const TaskProxy &task_proxy,
    const LandmarkNode &node,
    utils::LogProxy &log) {
    if (log.is_at_least_debug()) {
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
}

static void dump_edge(int from, int to, EdgeType edge, utils::LogProxy &log) {
    if (log.is_at_least_debug()) {
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
        }
        cout << "];\n";
    }
}

void dump_landmark_graph(
    const TaskProxy &task_proxy,
    const LandmarkGraph &graph,
    utils::LogProxy &log) {
    if (log.is_at_least_debug()) {
        log << "Dumping landmark graph: " << endl;

        cout << "digraph G {\n";
        for (const unique_ptr<LandmarkNode> &node : graph.get_nodes()) {
            dump_node(task_proxy, *node, log);
            for (const auto &child : node->children) {
                const LandmarkNode *child_node = child.first;
                const EdgeType &edge = child.second;
                dump_edge(node->get_id(), child_node->get_id(), edge, log);
            }
        }
        cout << "}" << endl;
        log << "Landmark graph end." << endl;
    }
}
}
