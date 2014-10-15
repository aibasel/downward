#include "pdb_heuristic.h"

#include "match_tree.h"
#include "util.h"

#include "../globals.h"
#include "../global_operator.h"
#include "../global_state.h"
#include "../plugin.h"
#include "../priority_queue.h"
#include "../timer.h"
#include "../utilities.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <limits>
#include <string>
#include <vector>

using namespace std;

AbstractOperator::AbstractOperator(const vector<pair<int, int> > &prev_pairs,
                                   const vector<pair<int, int> > &pre_pairs,
                                   const vector<pair<int, int> > &eff_pairs, int c,
                                   const vector<size_t> &hash_multipliers)
    : cost(c), regression_preconditions(prev_pairs) {
    regression_preconditions.insert(regression_preconditions.end(), eff_pairs.begin(), eff_pairs.end());
    sort(regression_preconditions.begin(), regression_preconditions.end()); // for MatchTree construction
    for (size_t i = 1; i < regression_preconditions.size(); ++i) {
        assert(regression_preconditions[i].first != regression_preconditions[i - 1].first);
    }
    hash_effect = 0;
    assert(pre_pairs.size() == eff_pairs.size());
    for (size_t i = 0; i < pre_pairs.size(); ++i) {
        int var = pre_pairs[i].first;
        assert(var == eff_pairs[i].first);
        int old_val = eff_pairs[i].second;
        int new_val = pre_pairs[i].second;
        assert(new_val != -1);
        size_t effect = (new_val - old_val) * hash_multipliers[var];
        hash_effect += effect;
    }
}

AbstractOperator::~AbstractOperator() {
}

void AbstractOperator::dump(const vector<int> &pattern) const {
    cout << "AbstractOperator:" << endl;
    cout << "Regression preconditions:" << endl;
    for (size_t i = 0; i < regression_preconditions.size(); ++i) {
        cout << "Variable: " << regression_preconditions[i].first << " (True name: "
             << g_variable_name[pattern[regression_preconditions[i].first]] << ", Index: "
             << i << ") Value: " << regression_preconditions[i].second << endl;
    }
    cout << "Hash effect:" << hash_effect << endl;
}

PDBHeuristic::PDBHeuristic(
    const Options &opts, bool dump,
    const vector<int> &operator_costs)
    : Heuristic(opts) {
    verify_no_axioms_no_conditional_effects();
    assert(operator_costs.empty() || operator_costs.size() == g_operators.size());

    Timer timer;
    set_pattern(opts.get_list<int>("pattern"), operator_costs);
    if (dump)
        cout << "PDB construction time: " << timer << endl;
}

PDBHeuristic::~PDBHeuristic() {
}

void PDBHeuristic::multiply_out(int pos, int cost, vector<pair<int, int> > &prev_pairs,
                                vector<pair<int, int> > &pre_pairs,
                                vector<pair<int, int> > &eff_pairs,
                                const vector<pair<int, int> > &effects_without_pre,
                                vector<AbstractOperator> &operators) {
    if (pos == static_cast<int>(effects_without_pre.size())) {
        // All effects without precondition have been checked: insert op.
        if (!eff_pairs.empty()) {
            operators.push_back(AbstractOperator(prev_pairs, pre_pairs, eff_pairs, cost, hash_multipliers));
        }
    } else {
        // For each possible value for the current variable, build an
        // abstract operator.
        int var = effects_without_pre[pos].first;
        int eff = effects_without_pre[pos].second;
        for (int i = 0; i < g_variable_domain[pattern[var]]; ++i) {
            if (i != eff) {
                pre_pairs.push_back(make_pair(var, i));
                eff_pairs.push_back(make_pair(var, eff));
            } else {
                prev_pairs.push_back(make_pair(var, i));
            }
            multiply_out(pos + 1, cost, prev_pairs, pre_pairs, eff_pairs,
                         effects_without_pre, operators);
            if (i != eff) {
                pre_pairs.pop_back();
                eff_pairs.pop_back();
            } else {
                prev_pairs.pop_back();
            }
        }
    }
}

void PDBHeuristic::build_abstract_operators(const GlobalOperator &op, int cost,
                                            const std::vector<int> &variable_to_index,
                                            vector<AbstractOperator> &operators) {
    vector<pair<int, int> > prev_pairs; // all variable value pairs that are a prevail condition
    vector<pair<int, int> > pre_pairs; // all variable value pairs that are a precondition (value != -1)
    vector<pair<int, int> > eff_pairs; // all variable value pairs that are an effect
    vector<pair<int, int> > effects_without_pre; // all variable value pairs that are a precondition (value = -1)

    const vector<GlobalCondition> &preconditions = op.get_preconditions();
    const vector<GlobalEffect> &effects = op.get_effects();
    vector<bool> has_precond_and_effect_on_var(g_variable_domain.size(), false);
    vector<bool> has_precondition_on_var(g_variable_domain.size(), false);

    for (size_t i = 0; i < preconditions.size(); ++i)
        has_precondition_on_var[preconditions[i].var] = true;

    for (size_t i = 0; i < effects.size(); ++i) {
        if (variable_to_index[effects[i].var] != -1) {
            if (has_precondition_on_var[effects[i].var]) {
                has_precond_and_effect_on_var[effects[i].var] = true;
                eff_pairs.push_back(make_pair(variable_to_index[effects[i].var], effects[i].val));
            } else {
                effects_without_pre.push_back(make_pair(variable_to_index[effects[i].var], effects[i].val));
            }
        }
    }
    for (size_t i = 0; i < preconditions.size(); ++i) {
        if (variable_to_index[preconditions[i].var] != -1) { // variable occurs in pattern
            if (has_precond_and_effect_on_var[preconditions[i].var]) {
                pre_pairs.push_back(make_pair(variable_to_index[preconditions[i].var], preconditions[i].val));
            } else {
                prev_pairs.push_back(make_pair(variable_to_index[preconditions[i].var], preconditions[i].val));
            }
        }
    }
    multiply_out(0, cost, prev_pairs, pre_pairs, eff_pairs, effects_without_pre, operators);
}

void PDBHeuristic::create_pdb(const std::vector<int> &operator_costs) {
    vector<int> variable_to_index(g_variable_name.size(), -1);
    for (size_t i = 0; i < pattern.size(); ++i) {
        variable_to_index[pattern[i]] = i;
    }

    // compute all abstract operators
    vector<AbstractOperator> operators;
    for (size_t i = 0; i < g_operators.size(); ++i) {
        const GlobalOperator &op = g_operators[i];
        int op_cost;
        if (operator_costs.empty()) {
            op_cost = get_adjusted_cost(op);
        } else {
            op_cost = operator_costs[i];
        }
        build_abstract_operators(op, op_cost, variable_to_index, operators);
    }

    // build the match tree
    MatchTree match_tree(pattern, hash_multipliers);
    for (size_t i = 0; i < operators.size(); ++i) {
        match_tree.insert(operators[i]);
    }

    // compute abstract goal var-val pairs
    vector<pair<int, int> > abstract_goal;
    for (size_t i = 0; i < g_goal.size(); ++i) {
        if (variable_to_index[g_goal[i].first] != -1) {
            abstract_goal.push_back(make_pair(variable_to_index[g_goal[i].first], g_goal[i].second));
        }
    }

    distances.reserve(num_states);
    AdaptiveQueue<size_t> pq; // (first implicit entry: priority,) second entry: index for an abstract state

    // initialize queue
    for (size_t state_index = 0; state_index < num_states; ++state_index) {
        if (is_goal_state(state_index, abstract_goal)) {
            pq.push(0, state_index);
            distances.push_back(0);
        } else {
            distances.push_back(numeric_limits<int>::max());
        }
    }

    // Dijkstra loop
    while (!pq.empty()) {
        pair<int, size_t> node = pq.pop();
        int distance = node.first;
        size_t state_index = node.second;
        if (distance > distances[state_index]) {
            continue;
        }

        // regress abstract_state
        vector<const AbstractOperator *> applicable_operators;
        match_tree.get_applicable_operators(state_index, applicable_operators);
        for (size_t i = 0; i < applicable_operators.size(); ++i) {
            size_t predecessor = state_index + applicable_operators[i]->get_hash_effect();
            int alternative_cost = distances[state_index] + applicable_operators[i]->get_cost();
            if (alternative_cost < distances[predecessor]) {
                distances[predecessor] = alternative_cost;
                pq.push(alternative_cost, predecessor);
            }
        }
    }
}

void PDBHeuristic::set_pattern(const vector<int> &pat,
                               const vector<int> &operator_costs) {
    assert(is_sorted_unique(pat));
    pattern = pat;
    hash_multipliers.reserve(pattern.size());
    num_states = 1;
    for (size_t i = 0; i < pattern.size(); ++i) {
        hash_multipliers.push_back(num_states);
        num_states *= g_variable_domain[pattern[i]];
    }
    create_pdb(operator_costs);
}

bool PDBHeuristic::is_goal_state(const size_t state_index, const vector<pair<int, int> > &abstract_goal) const {
    for (size_t i = 0; i < abstract_goal.size(); ++i) {
        int var = abstract_goal[i].first;
        int temp = state_index / hash_multipliers[var];
        int val = temp % g_variable_domain[pattern[var]];
        if (val != abstract_goal[i].second) {
            return false;
        }
    }
    return true;
}

size_t PDBHeuristic::hash_index(const GlobalState &state) const {
    size_t index = 0;
    for (size_t i = 0; i < pattern.size(); ++i) {
        index += hash_multipliers[i] * state[pattern[i]];
    }
    return index;
}

void PDBHeuristic::initialize() {
}

int PDBHeuristic::compute_heuristic(const GlobalState &state) {
    int h = distances[hash_index(state)];
    if (h == numeric_limits<int>::max())
        return DEAD_END;
    return h;
}

double PDBHeuristic::compute_mean_finite_h() const {
    double sum = 0;
    int size = 0;
    for (size_t i = 0; i < distances.size(); ++i) {
        if (distances[i] != numeric_limits<int>::max()) {
            sum += distances[i];
            ++size;
        }
    }
    if (size == 0) { // All states are dead ends.
        return numeric_limits<double>::infinity();
    } else {
        return sum / size;
    }
}

bool PDBHeuristic::is_operator_relevant(const GlobalOperator &op) const {
    const std::vector<GlobalEffect> &effects = op.get_effects();
    for (size_t i = 0; i < effects.size(); ++i) {
        int var = effects[i].var;
        if (binary_search(pattern.begin(), pattern.end(), var)) {
            return true;
        }
    }
    return false;
}

static Heuristic *_parse(OptionParser &parser) {
    parser.document_synopsis("Pattern database heuristic", "TODO");
    parser.document_language_support("action costs", "supported");
    parser.document_language_support("conditional effects", "not supported");
    parser.document_language_support("axioms", "not supported");
    parser.document_property("admissible", "yes");
    parser.document_property("consistent", "yes");
    parser.document_property("safe", "yes");
    parser.document_property("preferred operators", "no");

    Heuristic::add_options_to_parser(parser);
    Options opts;
    parse_pattern(parser, opts);

    if (parser.dry_run())
        return 0;

    return new PDBHeuristic(opts);
}

static Plugin<Heuristic> _plugin("pdb", _parse);
