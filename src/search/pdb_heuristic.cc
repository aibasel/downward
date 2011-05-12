#include "pdb_heuristic.h"
#include "globals.h"
#include "match_tree.h"
#include "operator.h"
#include "plugin.h"
#include "priority_queue.h"
#include "raz_variable_order_finder.h"
#include "state.h"
#include "timer.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <limits>
#include <string>
#include <vector>

using namespace std;

// AbstractOperator ------------------------------------------------------------------------------.

AbstractOperator::AbstractOperator(const vector<pair<int, int> > &prev_pairs,
                                   const vector<pair<int, int> > &pre_pairs,
                                   const vector<pair<int, int> > &eff_pairs, int c,
                                   const vector<size_t> &hash_multipliers)
                                   : cost(c), regression_preconditions(prev_pairs) {
    for (size_t i = 0; i < eff_pairs.size(); ++i) {
        regression_preconditions.push_back(eff_pairs[i]);
    }
    sort(regression_preconditions.begin(), regression_preconditions.end()); // for MatchTree construction
    hash_effect = 0;
    assert(pre_pairs.size() == eff_pairs.size());
    for (size_t i = 0; i < pre_pairs.size(); ++i) {
        int var = pre_pairs[i].first;
        assert(var == eff_pairs[i].first);
        int old_val = eff_pairs[i].second;
        int new_val = pre_pairs[i].second;
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

// PDBHeuristic ---------------------------------------------------------------

PDBHeuristic::PDBHeuristic(
    const Options &opts, bool dump,
    const vector<int> &op_costs)
    : Heuristic(opts) {

    verify_no_axioms_no_cond_effects();

    if (op_costs.empty()) {
        operator_costs.reserve(g_operators.size());
        for (size_t i = 0; i < g_operators.size(); ++i)
            operator_costs.push_back(get_adjusted_cost(g_operators[i]));
    } else {
        assert(op_costs.size() == g_operators.size());
        operator_costs = op_costs;
    }
    used_operators.resize(g_operators.size(), false);

    Timer timer;
    set_pattern(opts.get_list<int>("pattern"));
    if (dump)
        cout << "PDB construction time: " << timer << endl;
}

PDBHeuristic::~PDBHeuristic() {
}

void PDBHeuristic::verify_no_axioms_no_cond_effects() const {
    if (!g_axioms.empty()) {
        cerr << "Heuristic does not support axioms!" << endl << "Terminating." << endl;
        exit(1);
    }
    for (int i = 0; i < g_operators.size(); ++i) {
        const vector<PrePost> &pre_post = g_operators[i].get_pre_post();
        for (int j = 0; j < pre_post.size(); ++j) {
            const vector<Prevail> &cond = pre_post[j].cond;
            if (cond.empty())
                continue;
            // Accept conditions that are redundant, but nothing else.
            // In a better world, these would never be included in the
            // input in the first place.
            int var = pre_post[j].var;
            int pre = pre_post[j].pre;
            int post = pre_post[j].post;
            if (pre == -1 && cond.size() == 1 &&
                cond[0].var == var && cond[0].prev != post &&
                g_variable_domain[var] == 2)
                continue;
            
            cerr << "Heuristic does not support conditional effects "
                 << "(operator " << g_operators[i].get_name() << ")"
                 << endl << "Terminating." << endl;
            exit(1);
        }
    }
}

void PDBHeuristic::build_recursively(int pos, int op_no, int cost, vector<pair<int, int> > &prev_pairs,
                                     vector<pair<int, int> > &pre_pairs,
                                     vector<pair<int, int> > &eff_pairs,
                                     const vector<pair<int, int> > &effects_without_pre,
                                     vector<AbstractOperator> &operators) {
    if (pos == effects_without_pre.size()) {
        if (!eff_pairs.empty()) {
            operators.push_back(AbstractOperator(prev_pairs, pre_pairs, eff_pairs, cost, hash_multipliers));
            used_operators[op_no] = true;
        }
    } else {
        int var = effects_without_pre[pos].first;
        int eff = effects_without_pre[pos].second;
        for (size_t i = 0; i < g_variable_domain[pattern[var]]; ++i) {
            if (i != eff) {
                pre_pairs.push_back(make_pair(var, i));
                eff_pairs.push_back(make_pair(var, eff));
            } else {
                prev_pairs.push_back(make_pair(var, i));
            }
            build_recursively(pos+1, op_no, cost, prev_pairs, pre_pairs, eff_pairs,
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

void PDBHeuristic::build_abstract_operators(
    int op_no, vector<AbstractOperator> &operators) {
    const Operator &op = g_operators[op_no];
    vector<pair<int, int> > prev_pairs;
    vector<pair<int, int> > pre_pairs;
    vector<pair<int, int> > eff_pairs;
    vector<pair<int, int> > effects_without_pre;
    const vector<Prevail> &prevail = op.get_prevail();
    const vector<PrePost> &pre_post = op.get_pre_post();
    for (size_t i = 0; i < prevail.size(); ++i) {
        if (variable_to_index[prevail[i].var] != -1) { // variable occurs in pattern
            prev_pairs.push_back(make_pair(variable_to_index[prevail[i].var], prevail[i].prev));
        }
    }
    for (size_t i = 0; i < pre_post.size(); ++i) {
        if (variable_to_index[pre_post[i].var] != -1) {
            if (pre_post[i].pre != -1) {
                pre_pairs.push_back(make_pair(variable_to_index[pre_post[i].var], pre_post[i].pre));
                eff_pairs.push_back(make_pair(variable_to_index[pre_post[i].var], pre_post[i].post));
            } else {
                effects_without_pre.push_back(make_pair(variable_to_index[pre_post[i].var], pre_post[i].post));
            }
        }
    }
    build_recursively(0, op_no, operator_costs[op_no], prev_pairs, pre_pairs, eff_pairs, effects_without_pre, operators);
}

void PDBHeuristic::create_pdb() {
    vector<AbstractOperator> operators;
    for (size_t i = 0; i < g_operators.size(); ++i) {
        build_abstract_operators(i, operators);
    }

    MatchTree match_tree(pattern, hash_multipliers);
    for (size_t j = 0; j < operators.size(); ++j) {
        match_tree.insert(operators[j]);
    }

    // compute abstract goal var-val pairs
    vector<pair<int, int> > abstract_goal;
    for (size_t i = 0; i < g_goal.size(); ++i) {
        if (variable_to_index[g_goal[i].first] != -1) {
            abstract_goal.push_back(make_pair(variable_to_index[g_goal[i].first], g_goal[i].second));
        }
    }

    cout << "num_states = " << num_states << endl;
    distances.reserve(num_states);
    AdaptiveQueue<size_t> pq; // (first implicit entry: priority,) second entry: index for an abstract state

    // initialize queue
    for (size_t state_index = 0; state_index < num_states; ++state_index) {
        if (is_goal_state(state_index, abstract_goal)) {
            pq.push(0, state_index);
            distances.push_back(0);
        }
        else {
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

void PDBHeuristic::set_pattern(const vector<int> &pat) {
    pattern = pat;
    hash_multipliers.reserve(pattern.size());
    variable_to_index.resize(g_variable_name.size(), -1);
    num_states = 1;
    // int p = 1; ; same as num_states; incrementally computed!
    for (size_t i = 0; i < pattern.size(); ++i) {
        hash_multipliers.push_back(num_states);
        variable_to_index[pattern[i]] = i;
        num_states *= g_variable_domain[pattern[i]];
        //p *= g_variable_domain[pattern[i]];
    }
    create_pdb();
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

size_t PDBHeuristic::hash_index(const State &state) const {
    size_t index = 0;
    for (size_t i = 0; i < pattern.size(); ++i) {
        index += hash_multipliers[i] * state[pattern[i]];
    }
    return index;
}

/*AbstractState PDBHeuristic::inv_hash_index(const size_t index) const {
    vector<int> var_vals;
    var_vals.resize(pattern.size());
    for (size_t i = 0; i < pattern.size(); ++i) {
        int temp = index / hash_multipliers[i];
        var_vals[i] = temp % g_variable_domain[pattern[i]];
    }
    return AbstractState(var_vals);
}*/

void PDBHeuristic::initialize() {
}

int PDBHeuristic::compute_heuristic(const State &state) {
    int h = distances[hash_index(state)];
    if (h == numeric_limits<int>::max())
        return -1;
    return h;
}

void PDBHeuristic::dump() const {
    for (size_t i = 0; i < num_states; ++i) {
        //AbstractState abs_state = inv_hash_index(i);
        //abs_state.dump();
        cout << "h-value: " << distances[i] << endl;
    }
}

static ScalarEvaluator *_parse(OptionParser &parser) {
    parser.add_option<int>("max_states", 1000000, "maximum abstraction size");
    parser.add_list_option<int>("pattern", vector<int>(), "the pattern");
    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    vector<int> pattern = opts.get_list<int>("pattern");
    if (parser.dry_run() && !pattern.empty()) {
        sort(pattern.begin(), pattern.end());
        int old_size = pattern.size();
        vector<int>::const_iterator it = unique(pattern.begin(), pattern.end());
        pattern.resize(it - pattern.begin());
        if (pattern.size() != old_size)
            parser.error("there are duplicates of variables in the pattern");
        if (pattern[0] < 0)
            parser.error("there is a variable < 0");
        if (pattern[pattern.size() - 1] >= g_variable_domain.size())
            parser.error("there is a variable > number of variables");
    }
    if (opts.get<int>("max_states") < 1)
        parser.error("abstraction size must be at least 1");

    if (parser.dry_run())
        return 0;

    // if no pattern is specified as option
    if (pattern.empty()) {
        VariableOrderFinder vof(MERGE_LINEAR_GOAL_CG_LEVEL, 0.0);
        int var = vof.next();
        int num_states = g_variable_domain[var];
        while (num_states <= opts.get<int>("max_states")) {
            //cout << "Number of abstract states = " << num_states << endl;
            //cout << "Including variable: " << var << " (True name:" << g_variable_name[var] << ")" << endl;
            pattern.push_back(var);
            if (!vof.done()) {
                var = vof.next();
                num_states *= g_variable_domain[var];
            }
            else
                break;
        }
        opts.set("pattern", pattern);
    }

    return new PDBHeuristic(opts);
}

static Plugin<ScalarEvaluator> _plugin("pdb", _parse);
