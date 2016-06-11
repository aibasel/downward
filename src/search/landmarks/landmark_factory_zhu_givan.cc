#include "landmark_factory_zhu_givan.h"

#include "landmark_graph.h"

#include "../global_operator.h"
#include "../global_state.h"
#include "../globals.h"
#include "../option_parser.h"
#include "../plugin.h"

#include "../utils/language.h"

#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

using namespace std;

namespace landmarks {
LandmarkFactoryZhuGivan::LandmarkFactoryZhuGivan(const Options &opts)
    : LandmarkFactory(opts) {
}

void LandmarkFactoryZhuGivan::generate_landmarks(Exploration &exploration) {
    cout << "Generating landmarks using Zhu/Givan label propagation\n";

    compute_triggers();

    proposition_layer last_prop_layer = build_relaxed_plan_graph_with_labels();

    if (!satisfies_goal_conditions(last_prop_layer)) {
        cout << "Problem not solvable, even if relaxed.\n";
        return;
    }

    extract_landmarks(exploration, last_prop_layer);
}

bool LandmarkFactoryZhuGivan::satisfies_goal_conditions(
    const proposition_layer &layer) const {
    for (size_t i = 0; i < g_goal.size(); ++i)
        if (!layer[g_goal[i].first][g_goal[i].second].reached())
            return false;

    return true;
}

void LandmarkFactoryZhuGivan::extract_landmarks(
    Exploration &exploration,
    const proposition_layer &last_prop_layer) {
    utils::unused_variable(exploration);
    // insert goal landmarks and mark them as goals
    for (size_t i = 0; i < g_goal.size(); ++i) {
        LandmarkNode *lmp;
        if (lm_graph->simple_landmark_exists(g_goal[i])) {
            lmp = &lm_graph->get_simple_lm_node(g_goal[i]);
            lmp->in_goal = true;
        } else {
            lmp = &lm_graph->landmark_add_simple(g_goal[i]);
            lmp->in_goal = true;
        }
        // extract landmarks from goal labels
        const plan_graph_node &goal_node =
            last_prop_layer[g_goal[i].first][g_goal[i].second];

        assert(goal_node.reached());

        for (lm_set::const_iterator it = goal_node.labels.begin(); it
             != goal_node.labels.end(); ++it) {
            if (*it == g_goal[i]) // ignore label on itself
                continue;
            LandmarkNode *node;
            // Add new landmarks
            if (!lm_graph->simple_landmark_exists(*it)) {
                node = &lm_graph->landmark_add_simple(*it);

                // if landmark is not in the initial state,
                // relaxed_task_solvable() should be false
                assert(hacked_initial_state()[it->first] == it->second ||
                       !relaxed_task_solvable(exploration, true, node));
            } else
                node = &lm_graph->get_simple_lm_node(*it);
            // Add order: *it ->_{nat} g_goal[i]
            assert(node->parents.find(lmp) == node->parents.end());
            assert(lmp->children.find(node) == lmp->children.end());
            edge_add(*node, *lmp, EdgeType::natural);
        }
    }
}

LandmarkFactoryZhuGivan::proposition_layer LandmarkFactoryZhuGivan::build_relaxed_plan_graph_with_labels() const {
    assert(!triggers.empty());

    proposition_layer current_prop_layer;
    unordered_set<int> triggered(g_operators.size() + g_axioms.size());

    // set initial layer
    const GlobalState &initial_state = hacked_initial_state();
    current_prop_layer.resize(g_variable_domain.size());
    for (size_t i = 0; i < g_variable_domain.size(); ++i) {
        current_prop_layer[i].resize(g_variable_domain[i]);

        // label nodes from initial state
        const int ival = initial_state[i];
        current_prop_layer[i][ival].labels.insert(make_pair(i, ival));

        triggered.insert(triggers[i][ival].begin(), triggers[i][ival].end());
    }
    // Operators without preconditions do not propagate labels. So if they have
    // no conditional effects, is only necessary to apply them once. (If they
    // have conditional effects, they will be triggered at later stages again).
    triggered.insert(operators_without_preconditions.begin(),
                     operators_without_preconditions.end());

    bool changes = true;
    while (changes) {
        proposition_layer next_prop_layer(current_prop_layer);
        unordered_set<int> next_triggered;
        changes = false;
        for (int op_index : triggered) {
            const GlobalOperator &op = lm_graph->get_operator_for_lookup_index(op_index);
            if (operator_applicable(op, current_prop_layer)) {
                lm_set changed = apply_operator_and_propagate_labels(op,
                                                                     current_prop_layer, next_prop_layer);
                if (!changed.empty()) {
                    changes = true;
                    for (lm_set::const_iterator it2 = changed.begin(); it2
                         != changed.end(); ++it2)
                        next_triggered.insert(
                            triggers[it2->first][it2->second].begin(),
                            triggers[it2->first][it2->second].end());
                }
            }
        }
        current_prop_layer = next_prop_layer;
        triggered = next_triggered;
    }

    return current_prop_layer;
}

bool LandmarkFactoryZhuGivan::operator_applicable(const GlobalOperator &op,
                                                  const proposition_layer &state) const {
    // test preconditions
    const vector<GlobalCondition> &preconditions = op.get_preconditions();
    for (size_t i = 0; i < preconditions.size(); ++i)
        if (!state[preconditions[i].var][preconditions[i].val].reached())
            return false;
    return true;
}

bool LandmarkFactoryZhuGivan::operator_cond_effect_fires(
    const vector<GlobalCondition> &cond, const proposition_layer &state) const {
    for (size_t i = 0; i < cond.size(); ++i)
        if (!state[cond[i].var][cond[i].val].reached())
            return false;
    return true;
}

static lm_set _union(const lm_set &a, const lm_set &b) {
    if (a.size() < b.size())
        return _union(b, a);

    lm_set result = a;

    for (lm_set::const_iterator it = b.begin(); it != b.end(); ++it)
        result.insert(*it);
    return result;
}

static lm_set _intersection(const lm_set &a, const lm_set &b) {
    if (a.size() > b.size())
        return _intersection(b, a);

    lm_set result;

    for (lm_set::const_iterator it = a.begin(); it != a.end(); ++it)
        if (b.find(*it) != b.end())
            result.insert(*it);
    return result;
}

lm_set LandmarkFactoryZhuGivan::union_of_precondition_labels(const GlobalOperator &op,
                                                             const proposition_layer &current) const {
    lm_set result;

    // TODO This looks like an O(n^2) algorithm where O(n log n) would do, a
    // bit like the Python string concatenation anti-pattern.
    const vector<GlobalCondition> &preconditions = op.get_preconditions();
    for (size_t i = 0; i < preconditions.size(); ++i)
        result =
            _union(result,
                   current[preconditions[i].var][preconditions[i].val].labels);

    return result;
}

lm_set LandmarkFactoryZhuGivan::union_of_condition_labels(
    const vector<GlobalCondition> &cond, const proposition_layer &current) const {
    lm_set result;
    for (size_t i = 0; i < cond.size(); ++i)
        result = _union(result, current[cond[i].var][cond[i].val].labels);

    return result;
}

static bool _propagate_labels(lm_set &labels, const lm_set &new_labels,
                              const pair<int, int> &prop) {
    lm_set old_labels = labels;

    if (!labels.empty()) {
        labels = _intersection(labels, new_labels);
    } else {
        labels = new_labels;
    }
    labels.insert(prop);

    assert(old_labels.empty() || old_labels.size() >= labels.size());
    assert(!labels.empty());
    // test if labels have changed or proposition has just been reached:
    // if it has just been reached:
    // (old_labels.size() == 0) && (labels.size() >= 1)
    // if old_labels.size() == labels.size(), then labels have not been refined
    // by intersection.
    if (old_labels.size() != labels.size())
        return true;

    return false;
}

lm_set LandmarkFactoryZhuGivan::apply_operator_and_propagate_labels(
    const GlobalOperator &op, const proposition_layer &current,
    proposition_layer &next) const {
    assert(operator_applicable(op, current));

    lm_set result;
    lm_set precond_label_union = union_of_precondition_labels(op, current);

    const vector<GlobalEffect> &effects = op.get_effects();
    for (size_t i = 0; i < effects.size(); ++i) {
        const int var = effects[i].var;
        const int post = effects[i].val;

        if (next[var][post].labels.size() == 1)
            continue;

        if (operator_cond_effect_fires(effects[i].conditions, current)) {
            const lm_set precond_label_union_with_condeff = _union(
                precond_label_union, union_of_condition_labels(
                    effects[i].conditions, current));         // NOTE: this equals precond_label_union, if effects[i] is
            // not a conditional effect

            if (_propagate_labels(next[var][post].labels,
                                  precond_label_union_with_condeff, make_pair(var, post)))
                result.insert(make_pair(var, post));
        }
    }

    return result;
}

void LandmarkFactoryZhuGivan::compute_triggers() {
    assert(triggers.empty());

    // initialize empty triggers
    triggers.resize(g_variable_domain.size());
    for (size_t i = 0; i < g_variable_domain.size(); ++i)
        triggers[i].resize(g_variable_domain[i]);

    // compute triggers
    for (size_t i = 0; i < g_operators.size() + g_axioms.size(); ++i) {
        // collect possible triggers first
        lm_set t;

        const GlobalOperator &op = lm_graph->get_operator_for_lookup_index(i);
        const vector<GlobalCondition> &preconditions = op.get_preconditions();
        for (size_t j = 0; j < preconditions.size(); ++j)
            t.insert(make_pair(preconditions[j].var, preconditions[j].val));

        const vector<GlobalEffect> &effects = op.get_effects();
        for (size_t j = 0; j < effects.size(); ++j) {
            const vector<GlobalCondition> &cond = effects[j].conditions;
            for (size_t k = 0; k < cond.size(); ++k)
                t.insert(make_pair(cond[k].var, cond[k].val));
        }
        if (op.get_preconditions().empty())
            operators_without_preconditions.push_back(i);

        // add operator to triggers vector
        for (lm_set::const_iterator it = t.begin(); it != t.end(); ++it)
            triggers[it->first][it->second].push_back(i);
    }
}

bool LandmarkFactoryZhuGivan::supports_conditional_effects() const {
    return true;
}

static LandmarkFactory *_parse(OptionParser &parser) {
    parser.document_synopsis(
        "Zhu/Givan Landmarks",
        "The landmark generation method introduced by "
        "Zhu & Givan (ICAPS 2003 Doctoral Consortium).");
    parser.document_note("Relevant options", "reasonable_orders, no_orders");
    _add_options_to_parser(parser);
    Options opts = parser.parse();

    // TODO: Make sure that conditional effects are indeed supported.
    parser.document_language_support("conditional_effects",
                                     "We think they are supported, but this "
                                     "is not 100% sure.");

    if (parser.dry_run()) {
        return nullptr;
    } else {
        return new LandmarkFactoryZhuGivan(opts);
    }
}

static Plugin<LandmarkFactory> _plugin("lm_zg", _parse);
}
