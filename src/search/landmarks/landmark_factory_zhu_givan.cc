#include "landmark_factory_zhu_givan.h"
#include "landmark_graph.h"
#include "../operator.h"
#include "../state.h"
#include "../globals.h"
#include "../option_parser.h"
#include "../plugin.h"

#include <iostream>
#include <vector>
#include <utility>
#include <ext/hash_map>

using namespace __gnu_cxx;

LandmarkFactoryZhuGivan::LandmarkFactoryZhuGivan(const Options &opts)
    : LandmarkFactory(opts) {
}

void LandmarkFactoryZhuGivan::generate_landmarks() {
    cout << "Generating landmarks using Zhu/Givan label propagation\n";

    compute_triggers();

    proposition_layer last_prop_layer = build_relaxed_plan_graph_with_labels();

    if (!satisfies_goal_conditions(last_prop_layer)) {
        cout << "Problem not solvable, even if relaxed.\n";
        return;
    }

    extract_landmarks(last_prop_layer);
}

bool LandmarkFactoryZhuGivan::satisfies_goal_conditions(
    const proposition_layer &layer) const {
    for (unsigned i = 0; i < g_goal.size(); i++)
        if (!layer[g_goal[i].first][g_goal[i].second].reached())
            return false;

    return true;
}

void LandmarkFactoryZhuGivan::extract_landmarks(
    const proposition_layer &last_prop_layer) {
    // insert goal landmarks and mark them as goals
    for (unsigned i = 0; i < g_goal.size(); i++) {
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
             != goal_node.labels.end(); it++) {
            if (*it == g_goal[i]) // ignore label on itself
                continue;
            LandmarkNode *node;
            // Add new landmarks
            if (!lm_graph->simple_landmark_exists(*it)) {
                node = &lm_graph->landmark_add_simple(*it);

                // if landmark is not in the initial state,
                // relaxed_task_solvable() should be false
                assert((*g_initial_state)[it->first] == it->second ||
                       !relaxed_task_solvable(true, node));
            } else
                node = &lm_graph->get_simple_lm_node(*it);
            // Add order: *it ->_{nat} g_goal[i]
            assert(node->parents.find(lmp) == node->parents.end());
            assert(lmp->children.find(node) == lmp->children.end());
            edge_add(*node, *lmp, natural);
        }
    }
}

LandmarkFactoryZhuGivan::proposition_layer LandmarkFactoryZhuGivan::build_relaxed_plan_graph_with_labels() const {
    assert(!triggers.empty());

    proposition_layer current_prop_layer;
    hash_set<int> triggered(g_operators.size() + g_axioms.size());

    // set initial layer
    current_prop_layer.resize(g_variable_domain.size());
    for (unsigned i = 0; i < g_variable_domain.size(); i++) {
        current_prop_layer[i].resize(g_variable_domain[i]);

        // label nodes from initial state
        const int ival = (*g_initial_state)[i];
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
        hash_set<int> next_triggered;
        changes = false;
        for (hash_set<int>::const_iterator it = triggered.begin(); it
             != triggered.end(); it++) {
            const Operator &op = lm_graph->get_operator_for_lookup_index(*it);
            if (operator_applicable(op, current_prop_layer)) {
                lm_set changed = apply_operator_and_propagate_labels(op,
                                                                     current_prop_layer, next_prop_layer);
                if (!changed.empty()) {
                    changes = true;
                    for (lm_set::const_iterator it2 = changed.begin(); it2
                         != changed.end(); it2++)
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

bool LandmarkFactoryZhuGivan::operator_applicable(const Operator &op,
                                                  const proposition_layer &state) const {
    // test preconditions
    const vector<Prevail> &prevail = op.get_prevail();
    for (unsigned i = 0; i < prevail.size(); i++)
        if (!state[prevail[i].var][prevail[i].prev].reached())
            return false;

    const vector<PrePost> &prepost = op.get_pre_post();
    for (unsigned i = 0; i < prepost.size(); i++)
        if (prepost[i].pre != -1
            && !state[prepost[i].var][prepost[i].pre].reached())
            return false;

    return true;
}

bool LandmarkFactoryZhuGivan::operator_cond_effect_fires(
    const vector<Prevail> &cond, const proposition_layer &state) const {
    for (unsigned i = 0; i < cond.size(); i++)
        if (!state[cond[i].var][cond[i].prev].reached())
            return false;
    return true;
}

static lm_set _union(const lm_set &a, const lm_set &b) {
    if (a.size() < b.size())
        return _union(b, a);

    lm_set result = a;

    for (lm_set::const_iterator it = b.begin(); it != b.end(); it++)
        result.insert(*it);
    return result;
}

static lm_set _intersection(const lm_set &a, const lm_set &b) {
    if (a.size() > b.size())
        return _intersection(b, a);

    lm_set result;

    for (lm_set::const_iterator it = a.begin(); it != a.end(); it++)
        if (b.find(*it) != b.end())
            result.insert(*it);
    return result;
}

lm_set LandmarkFactoryZhuGivan::union_of_precondition_labels(const Operator &op,
                                                             const proposition_layer &current) const {
    lm_set result;

    const vector<Prevail> &prevail = op.get_prevail();
    for (unsigned i = 0; i < prevail.size(); i++)
        result =
            _union(result,
                   current[prevail[i].var][prevail[i].prev].labels);

    const vector<PrePost> &prepost = op.get_pre_post();
    for (unsigned i = 0; i < prepost.size(); i++)
        if (prepost[i].pre != -1)
            result = _union(result,
                            current[prepost[i].var][prepost[i].pre].labels);

    return result;
}

lm_set LandmarkFactoryZhuGivan::union_of_condition_labels(
    const vector<Prevail> &cond, const proposition_layer &current) const {
    lm_set result;
    for (unsigned i = 0; i < cond.size(); i++)
        result = _union(result, current[cond[i].var][cond[i].prev].labels);

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
    const Operator &op, const proposition_layer &current,
    proposition_layer &next) const {
    assert(operator_applicable(op, current));

    lm_set result;
    lm_set precond_label_union = union_of_precondition_labels(op, current);

    const vector<PrePost> &prepost = op.get_pre_post();
    for (int i = 0; i < prepost.size(); i++) {
        const int var = prepost[i].var;
        const int post = prepost[i].post;

        if (next[var][post].labels.size() == 1)
            continue;

        if (operator_cond_effect_fires(prepost[i].cond, current)) {
            const lm_set precond_label_union_with_condeff = _union(
                precond_label_union, union_of_condition_labels(
                    prepost[i].cond, current));         // NOTE: this equals precond_label_union, if prepost[i] is
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
    for (unsigned i = 0; i < g_variable_domain.size(); i++)
        triggers[i].resize(g_variable_domain[i]);

    // compute triggers
    for (unsigned i = 0; i < g_operators.size() + g_axioms.size(); i++) {
        // collect possible triggers first
        lm_set t;
        bool has_cond = false;

        const Operator &op = lm_graph->get_operator_for_lookup_index(i);
        const vector<Prevail> &prevail = op.get_prevail();
        for (unsigned j = 0; j < prevail.size(); j++) {
            t.insert(make_pair(prevail[j].var, prevail[j].prev));
            has_cond = true;
        }

        const vector<PrePost> &prepost = op.get_pre_post();
        for (unsigned j = 0; j < prepost.size(); j++) {
            if (prepost[j].pre != -1) {
                t.insert(make_pair(prepost[j].var, prepost[j].pre));
                has_cond = true;
            }
            const vector<Prevail> &cond = prepost[j].cond;
            for (unsigned k = 0; k < cond.size(); k++) {
                t.insert(make_pair(cond[k].var, cond[k].prev));
            }
        }
        if (!has_cond) // no preconditions
            operators_without_preconditions.push_back(i);

        // add operator to triggers vector
        for (lm_set::const_iterator it = t.begin(); it != t.end(); it++)
            triggers[it->first][it->second].push_back(i);
    }
}

static LandmarkGraph *_parse(OptionParser &parser) {
    LandmarkGraph::add_options_to_parser(parser);
    Options opts = parser.parse();

    if (parser.dry_run()) {
        return 0;
    } else {
        opts.set<Exploration *>("explor", new Exploration(opts));
        LandmarkFactoryZhuGivan lm_graph_factory(opts);
        LandmarkGraph *graph = lm_graph_factory.compute_lm_graph();
        return graph;
    }
}

static Plugin<LandmarkGraph> _plugin("lm_zg", _parse);
