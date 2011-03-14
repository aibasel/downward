#include "pdb_heuristic.h"
#include "abstract_state_iterator.h"
#include "globals.h"
#include "operator.h"
#include "plugin.h"
#include "raz_variable_order_finder.h"
#include "state.h"
#include "timer.h"

#include <cassert>
#include <cstdlib>
#include <limits>
#include <queue>
#include <string>
#include <vector>

using namespace std;

// AbstractOperator -------------------------------------------------------------------------------

AbstractOperator::AbstractOperator(const Operator &o, const vector<int> &var_to_index) : cost(o.get_cost()) {
    const vector<Prevail> &prevail = o.get_prevail();
    const vector<PrePost> &pre_post = o.get_pre_post();
    for (size_t j = 0; j < prevail.size(); ++j) {
        if (var_to_index[prevail[j].var] != -1) {
            conditions.push_back(make_pair(var_to_index[prevail[j].var], prevail[j].prev));
        }
    }
    for (size_t j = 0; j < pre_post.size(); ++j) {
        if (var_to_index[pre_post[j].var] != -1) {
            if (pre_post[j].pre != -1)
                conditions.push_back(make_pair(var_to_index[pre_post[j].var], pre_post[j].pre));
            effects.push_back(make_pair(var_to_index[pre_post[j].var], pre_post[j].post));
        }
    }
}

AbstractOperator::AbstractOperator(const vector<pair<int, int> > &pre_pairs,
                                   const vector<pair<int, int> > &eff_pairs, int c)
                                   : cost(c), conditions(pre_pairs), effects(eff_pairs) {
}

AbstractOperator::~AbstractOperator() {
}

void AbstractOperator::dump(const vector<int> &pattern) const {
    cout << "AbstractOperator:" << endl;
    cout << "Conditions:" << endl;
    for (size_t i = 0; i < conditions.size(); ++i) {
        cout << "Variable: " << conditions[i].first << " (True name: " 
        << g_variable_name[pattern[conditions[i].first]] << ") Value: " << conditions[i].second << endl;
    }
    cout << "Effects:" << endl;
    for (size_t i = 0; i < effects.size(); ++i) {
        cout << "Variable: " << effects[i].first << " (True name: " 
        << g_variable_name[pattern[effects[i].first]] << ") Value: " << effects[i].second << endl;
    }
}

// AbstractState ----------------------------------------------------------------------------------

AbstractState::AbstractState(const vector<int> &var_vals) : variable_values(var_vals) {
}

AbstractState::AbstractState(const State &state, const vector<int> &pattern) {
    for (size_t i = 0; i < pattern.size(); ++i) {
        variable_values.push_back(state[pattern[i]]);
    }
}

AbstractState::~AbstractState() {
}

bool AbstractState::is_applicable(const AbstractOperator &op) const {
    const vector<pair<int, int> > &conditions = op.get_conditions();
    for (size_t i = 0; i < conditions.size(); ++i) {
        if (variable_values[conditions[i].first] != conditions[i].second)
            return false;
    }
    return true;
}

void AbstractState::apply_operator(const AbstractOperator &op) {
    assert(is_applicable(op));
    const vector<pair<int, int> > &effects = op.get_effects();
    for (size_t i = 0; i < effects.size(); ++i) {
        int var = effects[i].first;
        int val = effects[i].second;
        variable_values[var] = val;
    }
}

bool AbstractState::is_goal_state(const vector<pair<int, int> > &abstract_goal) const {
    for (size_t i = 0; i < abstract_goal.size(); ++i) {
        if (variable_values[abstract_goal[i].first] != abstract_goal[i].second) {
            return false;
        }
    }
    return true;
}

void AbstractState::dump(const vector<int> &pattern) const {
    cout << "AbstractState: " << endl;
    for (size_t i = 0; i < variable_values.size(); ++i) {
        cout << "Variable: " << pattern[i] << " (True name: " 
        << g_variable_name[pattern[i]] << ") Value: " << variable_values[i] << endl;
    }
}

// PDBHeuristic ---------------------------------------------------------------

static ScalarEvaluator *create(const vector<string> &config, int start, int &end, bool dry_run);
static ScalarEvaluatorPlugin plugin("pdb", create);

/*PDBHeuristic::PDBHeuristic(int max_abstract_states) {
    verify_no_axioms_no_cond_effects();
    Timer timer;
    generate_pattern(max_abstract_states);
    cout << "PDB construction time: " << timer << endl;
}*/

PDBHeuristic::PDBHeuristic(const vector<int> &pattern)
    : Heuristic(HeuristicOptions()) {
    verify_no_axioms_no_cond_effects();
    Timer timer;
    set_pattern(pattern);
    cout << "PDB construction time: " << timer << endl;
}

PDBHeuristic::~PDBHeuristic() {}

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

void PDBHeuristic::build_recursively(int pos, int cost, vector<pair<int, int> > pre_pairs,
                                      vector<pair<int, int> > eff_pairs,
                                      const vector<pair<int, int> > effects_without_pre,
                                      vector<AbstractOperator> &operators) {
    if (pos == effects_without_pre.size()) {
        if (!eff_pairs.empty()) {
            operators.push_back(AbstractOperator(pre_pairs, eff_pairs, cost));
        }
    } else {
        int var = effects_without_pre[pos].first;
        int eff = effects_without_pre[pos].second;
        for (size_t i = 0; i < g_variable_domain[pattern[var]]; ++i) {
            pre_pairs.push_back(make_pair(var, i));
            if (i != eff)
                eff_pairs.push_back(make_pair(var, eff));
            build_recursively(pos+1, cost, pre_pairs, eff_pairs,
                                effects_without_pre, operators);
            if (i != eff)
                eff_pairs.pop_back();
            pre_pairs.pop_back();
        }
    }
}

void PDBHeuristic::build_abstract_operators(const Operator &op, vector<AbstractOperator> &operators) {
    vector<pair<int, int> > pre_pairs;
    vector<pair<int, int> > eff_pairs;
    vector<pair<int, int> > effects_without_pre;
    const vector<Prevail> &prevail = op.get_prevail();
    const vector<PrePost> &pre_post = op.get_pre_post();
    for (size_t i = 0; i < prevail.size(); ++i) {
        if (variable_to_index[prevail[i].var] != -1) { // variable occurs in pattern
            pre_pairs.push_back(make_pair(variable_to_index[prevail[i].var], prevail[i].prev));
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
    build_recursively(0, op.get_cost(), pre_pairs, eff_pairs, effects_without_pre, operators);
}

void PDBHeuristic::create_pdb_new() {
    vector<AbstractOperator> operators;
    //size_t index = 0;
    for (size_t i = 0; i < g_operators.size(); ++i) {
        build_abstract_operators(g_operators[i], operators);
        /*g_operators[i].dump();
        cout << "abstracted and multiplied out operators:" << endl;
        while (index < operators.size()) {
            operators[i].dump(pattern);
            ++index;
        }*/
    }

    // assertion tests for domain in which no operators with pre = -1 exist
    // in this case, both methods should generate the exact same abstract operators
    vector<AbstractOperator> operators2;
    for (size_t i = 0; i < g_operators.size(); ++i) {
        AbstractOperator ao(g_operators[i], variable_to_index);
        if (!ao.get_effects().empty())
            operators2.push_back(ao);
    }
    assert(operators.size() == operators2.size());
    for (size_t i = 0; i < operators.size(); ++i) {
        cout << "i = " << i << endl;
        const vector<pair<int, int> > &conditions = operators[i].get_conditions();
        const vector<pair<int, int> > &conditions2 = operators2[i].get_conditions();
        assert(conditions.size() == conditions2.size());
        for (size_t j = 0; j < conditions.size(); ++j) {
            assert(conditions[j].first == conditions2[j].first);
            assert(conditions[j].second == conditions2[j].second);
        }
        const vector<pair<int, int> > &effects = operators[i].get_effects();
        const vector<pair<int, int> > &effects2 = operators2[i].get_effects();
        assert(effects.size() == effects2.size());
        for (size_t j = 0; j < effects.size(); ++j) {
            assert(effects[j].first == effects2[j].first);
            assert(effects[j].second == effects2[j].second);
        }
    }

    vector<pair<int, int> > abstracted_goal;
    for (size_t i = 0; i < g_goal.size(); ++i) {
        if (variable_to_index[g_goal[i].first] != -1) {
            abstracted_goal.push_back(make_pair(variable_to_index[g_goal[i].first], g_goal[i].second));
        }
    }

    vector<int> distances2;
    distances2.reserve(num_states);
    // first entry: priority, second entry: index for an abstract state
    priority_queue<pair<int, size_t>, vector<pair<int, size_t> >, greater<pair<int, size_t> > > pq;

    vector<int> ranges;
    for (size_t i = 0; i < pattern.size(); ++i) {
        ranges.push_back(g_variable_domain[pattern[i]]);
    }
    for (AbstractStateIterator it(ranges); !it.is_at_end(); it.next()) {
        AbstractState abstract_state(it.get_current());
        int counter = it.get_counter();
        assert(hash_index(abstract_state) == counter);

        if (abstract_state.is_goal_state(abstracted_goal)) {
            pq.push(make_pair(0, counter));
            distances2.push_back(0);
        }
        else {
            distances2.push_back(numeric_limits<int>::max());
        }
    }

    while (!pq.empty()) {
        pair<int, int> node = pq.top();
        pq.pop();
        int distance = node.first;
        size_t state_index = node.second;
        if (distance > distances2[state_index])
            continue;
        AbstractState abstract_state = inv_hash_index(state_index);
        // regress abstract_state
        for (size_t i = 0; i < operators.size(); ++i) {
            const vector<pair<int, int> > &pre = operators[i].get_conditions();
            const vector<pair<int, int> > &eff = operators[i].get_effects();
            int cost = operators[i].get_cost();
            bool eff_in_state = true;
            for (size_t j = 0; j < eff.size(); ++j) {
                if (abstract_state[eff[j].first] != eff[j].second) {
                    eff_in_state = false;
                    break;
                }
            }
            if (eff_in_state) {
                // TODO: ask Malte if there is a difference between &var_vals or var_vals
                // if the methods anyways returns a reference
                vector<int> var_vals = abstract_state.get_var_vals();
                for (size_t j = 0; j < var_vals.size(); ++j) {
                    for (size_t k = 0; k < pre.size(); ++k) {
                        if (j == pre[k].first)
                            var_vals[j] = pre[k].second;
                    }
                }
                AbstractState regressed_state(var_vals);
                size_t predecessor = hash_index(regressed_state);
                
                // old pq-code
                int alternative_cost = distances2[state_index] + cost;
                if (alternative_cost < distances2[predecessor]) {
                    distances2[predecessor] = alternative_cost;
                    pq.push(make_pair(alternative_cost, predecessor));
                }
            }
        }
    }
/*
    assert(distances2.size() == distances.size());
    for (size_t i = 0; i < distances2.size(); ++i) {
        if (distances2[i] != distances[i]) {
            cout << "distances2[" << i << "] = " << distances2[i] << " != " << distances[i] << " = distances[" << i << "]" << endl;
        }
        assert(distances2[i] == distances[i]);
    }
    cout << "assertion checked - distances correctly calculated" << endl;*/
    cout << "done creating." << endl;
    exit(2);
}

void PDBHeuristic::create_pdb() {
    vector<AbstractOperator> operators;
    for (size_t i = 0; i < g_operators.size(); ++i) {
        AbstractOperator ao(g_operators[i], variable_to_index);
        if (!ao.get_effects().empty())
            operators.push_back(ao);
    }
    
    vector<pair<int, int> > abstracted_goal;
    for (size_t i = 0; i < g_goal.size(); ++i) {
        if (variable_to_index[g_goal[i].first] != -1) {
            abstracted_goal.push_back(make_pair(variable_to_index[g_goal[i].first], g_goal[i].second));
        }
    }
    
    vector<vector<Edge> > back_edges;
    back_edges.resize(num_states);
    distances.reserve(num_states);
    // first entry: priority, second entry: index for an abstract state
    priority_queue<pair<int, size_t>, vector<pair<int, size_t> >, greater<pair<int, size_t> > > pq;
    
    vector<int> ranges;
    for (size_t i = 0; i < pattern.size(); ++i) {
        ranges.push_back(g_variable_domain[pattern[i]]);
    }
    for (AbstractStateIterator it(ranges); !it.is_at_end(); it.next()) {
        AbstractState abstract_state(it.get_current());
        int counter = it.get_counter();
        assert(hash_index(abstract_state) == counter);
        
        if (abstract_state.is_goal_state(abstracted_goal)) {
            pq.push(make_pair(0, counter));
            distances.push_back(0);
        }
        else {
            distances.push_back(numeric_limits<int>::max());
        }
        //abstract_state.dump(pattern);
        //cout << "successor states:" << endl;
        for (size_t j = 0; j < operators.size(); ++j) {
            if (abstract_state.is_applicable(operators[j])) {
                //cout << "applying operator:" << endl;
                //operators[j].dump(pattern);
                AbstractState next_state = abstract_state;
                next_state.apply_operator(operators[j]);
                //next_state.dump(pattern);
                size_t state_index = hash_index(next_state);
                if (state_index == counter) {
                    //cout << "operator didn't lead to a new state, ignoring!" << endl;
                    continue;
                } else {
                    assert(counter != state_index);
                    back_edges[state_index].push_back(Edge(operators[j].get_cost(), counter));
                }
            }
        }
        //cout << endl;
    }
    
    while (!pq.empty()) {
        pair<int, int> node = pq.top();
        pq.pop();
        int distance = node.first;
        size_t state_index = node.second;
        if (distance > distances[state_index])
            continue;
        const vector<Edge> &edges = back_edges[state_index];
        for (size_t i = 0; i < edges.size(); ++i) {
            size_t predecessor = edges[i].target;
            int cost = edges[i].cost;
            int alternative_cost = distances[state_index] + cost;
            if (alternative_cost < distances[predecessor]) {
                distances[predecessor] = alternative_cost;
                pq.push(make_pair(alternative_cost, predecessor));
            }
        }
    }
}

void PDBHeuristic::set_pattern(const vector<int> &pat) {
    pattern = pat;
    n_i.reserve(pattern.size());
    variable_to_index.resize(g_variable_name.size(), -1);
    num_states = 1;
    // int p = 1; ; same as num_states; incrementally computed!
    for (size_t i = 0; i < pattern.size(); ++i) {
        n_i.push_back(num_states);
        variable_to_index[pattern[i]] = i;
        num_states *= g_variable_domain[pattern[i]];
        //p *= g_variable_domain[pattern[i]];
    }
    //create_pdb();
    create_pdb_new();
}

size_t PDBHeuristic::hash_index(const AbstractState &abstract_state) const {
    size_t index = 0;
    for (int i = 0; i < pattern.size(); ++i) {
        index += n_i[i] * abstract_state[variable_to_index[pattern[i]]];
    }
    return index;
}

AbstractState PDBHeuristic::inv_hash_index(int index) const {
    vector<int> var_vals;
    var_vals.resize(pattern.size());
    for (int n = 1; n < pattern.size(); ++n) {
        int d = index % n_i[n];
        var_vals[variable_to_index[pattern[n - 1]]] = d / n_i[n - 1];
        index -= d;
    }
    var_vals[variable_to_index[pattern[pattern.size() - 1]]] = index / n_i[pattern.size() - 1];
    return AbstractState(var_vals);
}

void PDBHeuristic::initialize() {
    //cout << "Initializing pattern database heuristic..." << endl;
    //cout << "Didn't do anything. Done initializing." << endl;
}

int PDBHeuristic::compute_heuristic(const State &state) {
    int h = distances[hash_index(AbstractState(state, pattern))];;
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

ScalarEvaluator *create(const vector<string> &config, int start, int &end, bool dry_run) {
    int max_states = 1000000;
    if (config.size() > start + 2 && config[start + 1] == "(") {
        end = start + 2;
        if (config[end] != ")") {
            NamedOptionParser option_parser;
            option_parser.add_int_option("max_states", &max_states, "maximum abstraction size");
            option_parser.parse_options(config, end, end, dry_run);
            end++;
        }
        if (config[end] != ")")
            throw ParseError(end);
    } else {
        end = start;
    }

    if (max_states < 1) {
        cerr << "error: abstraction size must be at least 1" << endl;
        exit(2);
    }
    
    if (dry_run)
        return 0;
    
    vector<int> pattern;
#define DEBUG false
#if DEBUG
    // function tests
    // 1. blocks-7-2 test-pattern
    //int patt[] = {9, 10, 11, 12, 13, 14};
    //int patt[] = {13, 14};
    
    // 2. driverlog-6 test-pattern
    //int patt[] = {4, 5, 7, 9, 10, 11, 12};
    
    // 3. logistics00-6-2 test-pattern
    //int patt[] = {3, 4, 5, 6, 7, 8};
    
    // 4. blocks-9-0 test-pattern
    //int patt[] = {0};
    
    // 5. logistics00-5-1 test-pattern
    //int patt[] = {0, 1, 2, 3, 4, 5, 6, 7};
    
    // 6. some other test
    int patt[] = {9};
    
    pattern = vector<int>(patt, patt + sizeof(patt) / sizeof(int));
#else
    VariableOrderFinder vof(MERGE_LINEAR_GOAL_CG_LEVEL, 0.0);
    int var = vof.next();
    int num_states = g_variable_domain[var];
    while (num_states <= max_states) {
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
#endif
    return new PDBHeuristic(pattern);
}
