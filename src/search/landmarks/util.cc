#include "util.h"

#include "landmark.h"
#include "landmark_graph.h"

#include "../task_proxy.h"
#include "../utils/logging.h"

using namespace std;

namespace landmarks {
static bool condition_is_reachable(
    const ConditionsProxy &conditions, const vector<vector<bool>> &reached) {
    for (FactProxy condition : conditions) {
        if (!reached[condition.get_variable().get_id()][condition.get_value()]) {
            return false;
        }
    }
    return true;
}

/* Check whether operator `op` can possibly make `landmark` true in a
   relaxed task (as given by the reachability information in reached). */
bool possibly_reaches_landmark(const OperatorProxy &op,
                               const vector<vector<bool>> &reached,
                               const Landmark &landmark) {
    assert(!reached.empty());
    if (!condition_is_reachable(op.get_preconditions(), reached)) {
        // Operator `op` is not applicable.
        return false;
    }

    // Check whether an effect of `op` reaches an atom in `landmark`.
    EffectsProxy effects = op.get_effects();
    return any_of(begin(effects), end(effects), [&](const EffectProxy &effect) {
                      return landmark.contains(effect.get_fact().get_pair()) &&
                      condition_is_reachable(effect.get_conditions(), reached);
                  });
}

utils::HashSet<FactPair> get_intersection(
    const utils::HashSet<FactPair> &set1,
    const utils::HashSet<FactPair> &set2) {
    utils::HashSet<FactPair> intersection;
    for (const FactPair &atom : set1) {
        if (set2.contains(atom)) {
            intersection.insert(atom);
        }
    }
    return intersection;
}

void union_inplace(utils::HashSet<FactPair> &set1,
                   const utils::HashSet<FactPair> &set2) {
    set1.insert(set2.begin(), set2.end());
}

OperatorProxy get_operator_or_axiom(const TaskProxy &task_proxy,
                                    int op_or_axiom_id) {
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
  The functions below use cout on purpose for dumping a landmark graph.
  TODO: ideally, this should be written to a file or through a logger
  at least, but without the time and memory stamps.
*/
static void dump_node(
    const TaskProxy &task_proxy,
    const LandmarkNode &node,
    utils::LogProxy &log) {
    if (log.is_at_least_debug()) {
        const Landmark &landmark = node.get_landmark();
        char delimiter = landmark.type == DISJUNCTIVE ? '|' : '&';
        cout << "  lm" << node.get_id() << " [label=\"";
        bool first = true;
        for (const FactPair &atom : landmark.atoms) {
            if (!first) {
                cout << " " << delimiter << " ";
            }
            first = false;
            VariableProxy var = task_proxy.get_variables()[atom.var];
            cout << var.get_fact(atom.value).get_name();
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

static void dump_ordering(int from, int to, OrderingType type,
                          const utils::LogProxy &log) {
    if (log.is_at_least_debug()) {
        cout << "      lm" << from << " -> lm" << to << " [label=";
        switch (type) {
        case OrderingType::NECESSARY:
            cout << "\"nec\"";
            break;
        case OrderingType::GREEDY_NECESSARY:
            cout << "\"gn\"";
            break;
        case OrderingType::NATURAL:
            cout << "\"n\"";
            break;
        case OrderingType::REASONABLE:
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
        for (const auto &node : graph) {
            dump_node(task_proxy, *node, log);
            for (const auto &[child, type] : node->children) {
                dump_ordering(node->get_id(), child->get_id(), type, log);
            }
        }
        cout << "}" << endl;
        log << "Landmark graph end." << endl;
    }
}
}
