#include "pdb_heuristic.h"

#include "globals.h"
//#include "option_parser.h"
#include "plugin.h"
#include "state.h"
#include "operator.h"
#include <cassert>
#include <stdlib.h>

// AbstractState --------------------------------------------------------------

AbstractState::AbstractState(map<int, int> var_vals) {
    variable_values = var_vals;
}

AbstractState::AbstractState(const State &state, const vector<int> &pattern) {
    for (size_t i = 0; i < pattern.size(); i++) {
        int var = pattern[i];
        int val = state[var];
        variable_values[var] = val;
    }
}

map<int, int> AbstractState::get_variable_values() const {
    return variable_values;
}

bool AbstractState::is_goal(const vector<pair<int, int> > &abstract_goal) const {
    for (size_t i = 0; i < abstract_goal.size(); i++) {
        int var = abstract_goal[i].first;
        int val = abstract_goal[i].second;
        if (val != variable_values.find(var)->second) {
            return false;
        }
    }
    return true;
}

void AbstractState::dump() const {
    cout << "AbstractState: " << endl;
    map<int, int> copy_map(variable_values);
    for (map<int, int>::iterator it = copy_map.begin(); it != copy_map.end(); it++) {
        cout << "Variable: " <<  it->first << " (True name: " 
            << g_variable_name[it->first] << ") Value: " << it->second << endl;
    }
}

// AbstractOperator -----------------------------------------------------------

AbstractOperator::AbstractOperator(const Operator &o, const vector<int> &pattern) {
    const vector<PrePost> pre_post = o.get_pre_post();
    pre_effect = vector<PreEffect>();
    for (size_t i = 0; i < pre_post.size(); i++) {
        int var = pre_post[i].var;
        for (size_t j = 0; j < pattern.size(); j++) {
            if (var == pattern[j]) {
                int pre = pre_post[i].pre;
                int post = pre_post[i].post;
                pre_effect.push_back(make_pair(var, make_pair(pre, post)));
                break;
            }
        }
    }
}

bool AbstractOperator::is_applicable(const AbstractState &abstract_state) const {
    if (pre_effect.size() == 0)
        return false;
    for (size_t i = 0; i < pre_effect.size(); i++) {
        int var = pre_effect[i].first;
        int pre = pre_effect[i].second.first;
        assert(var >= 0 && var < g_variable_name.size());
        assert(pre == -1 || (pre >= 0 && pre < g_variable_domain[var]));
        if (!(pre == -1 || abstract_state.get_variable_values()[var] == pre))
            return false;
    }
    return true;
}

AbstractState AbstractOperator::apply_operator(const AbstractState &abstract_state) const {
    assert(is_applicable(abstract_state));
    map<int, int> variable_values = abstract_state.get_variable_values();
    for (size_t i = 0; i < pre_effect.size(); i++) {
        int var = pre_effect[i].first;
        int post = pre_effect[i].second.second;
        variable_values[var] = post;
    }
    return AbstractState(variable_values);
}

void AbstractOperator::dump() const {
    cout << "AbstractOperator: " << endl;
    for (size_t i = 0; i < pre_effect.size(); i++) {
        cout << "Variable: " << pre_effect[i].first << " (True name: " 
            << g_variable_name[pre_effect[i].first] << ") Pre: " << pre_effect[i].second.first 
            << " Post: "<< pre_effect[i].second.second << endl;
    }
}

// PDBAbstraction -------------------------------------------------------------

PDBAbstraction::PDBAbstraction(vector<int> pat) {
    pattern = pat;
    size = 1;
    for (size_t i = 0; i < pattern.size(); i++) {
        size *= g_variable_domain[pattern[i]];
    }
    distances = vector<int>(size, QUITE_A_LOT);
    back_edges = vector<vector<Edge> >(size);
}

void PDBAbstraction::create_pdb() {
    n_i = vector<int>(pattern.size());
    for (size_t i = 0; i < pattern.size(); i++) {
        int p = 1;
        for (int j = 0; j < i; j++) {
            p *= g_variable_domain[pattern[j]];
        }
        n_i[i] = p;
    }
    vector<AbstractOperator *> operators(g_operators.size());
    for (size_t i = 0; i < g_operators.size(); i++) {
        operators[i] = new AbstractOperator(g_operators[i], pattern);
    }
    vector<pair<int, int> > abstracted_goal;
    for (size_t i = 0; i < g_goal.size(); i++) {
        for (size_t j = 0; j < pattern.size(); j++) {
            if (g_goal[i].first == pattern[j]) {
                abstracted_goal.push_back(make_pair(g_goal[i].first, g_goal[i].second));
                break;
            }
        }
    }
    for (size_t i = 0; i < size; i++) {
        AbstractState abstract_state = inv_hash_index(i);
        if (abstract_state.is_goal(abstracted_goal)) {
            pq.push(make_pair(i, 0));
        }
        else {
            pq.push(make_pair(i, QUITE_A_LOT));
        }
        for (size_t j = 0; j < operators.size(); j++){
            if (operators[j]->is_applicable(abstract_state)) {;
                AbstractState next_state = operators[j]->apply_operator(abstract_state);
                int state_index = hash_index(next_state);
                back_edges[state_index].push_back(Edge(&(g_operators[j]), i));
            }
        }
    }
}

void PDBAbstraction::compute_goal_distances() {
    while (!pq.empty()) {
        Node node = pq.top();
        pq.pop();
        int state_index = node.first;
        int distance = node.second;
        if (distance > distances[state_index])
            continue;
        distances[state_index] = distance;
        vector<Edge> edges = back_edges[state_index];
        for (size_t i = 0; i < edges.size(); i++) {
            int predecessor = edges[i].target;
            int cost = edges[i].op->get_cost();
            int alternative = distances[state_index] + cost;
            if (alternative < distances[predecessor]) {
                distances[predecessor] = alternative;
                pq.push(make_pair(predecessor, alternative));
            }
        }
    }
}

int PDBAbstraction::hash_index(const AbstractState &abstract_state) const {
    int index = 0;
    for (int i = 0;i < pattern.size();i++) {
        index += n_i[i]*abstract_state.get_variable_values()[pattern[i]];
    }
    return index;
}

AbstractState PDBAbstraction::inv_hash_index(int index) const {
    map<int, int> var_vals;
    for (int n = 1;n < pattern.size();n++) {
        int d = index%n_i[n];
        var_vals[pattern[n-1]] = (int) d/n_i[n-1];
        index -= d;
    }
    var_vals[pattern[pattern.size()-1]] = (int) index/n_i[pattern.size()-1];
    return AbstractState(var_vals);
}

int PDBAbstraction::get_heuristic_value(const State &state) const {
    AbstractState abstract_state(state, pattern);
    int index = hash_index(abstract_state);
    return distances[index];
}

void PDBAbstraction::dump() const {
    for (size_t i = 0; i < size; i++) {
        AbstractState abs_state = inv_hash_index(i);
        abs_state.dump();
        cout << "h-value: " << distances[i] << endl;
    }
}

// PDBHeuristic ---------------------------------------------------------------

static ScalarEvaluatorPlugin pdb_heuristic_plugin(
    "pdb", PDBHeuristic::create);

PDBHeuristic::PDBHeuristic() {
}

PDBHeuristic::~PDBHeuristic() {
}

void PDBHeuristic::verify_no_axioms_no_cond_effects() const {
    if (!g_axioms.empty()) {
        cerr << "Heuristic does not support axioms!" << endl
        << "Terminating." << endl;
        exit(1);
    }
    for (int i = 0; i < g_operators.size(); i++) {
        const vector<PrePost> &pre_post = g_operators[i].get_pre_post();
        for (int j = 0; j < pre_post.size(); j++) {
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

void PDBHeuristic::initialize() {
    cout << "Initializing pattern database heuristic..." << endl;// << endl;
    verify_no_axioms_no_cond_effects();
    int patt[2] = {10, 11};
    vector<int> pattern(patt, patt + sizeof(patt) / sizeof(int));
    //cout << "Try creating a new PDBAbstraction" << endl << endl;
    pdb_abstraction = new PDBAbstraction(pattern);
    //cout << "Created new PDBAbstraction. Now calling create_pdb()" << endl << endl;
    pdb_abstraction->create_pdb();
    //cout << "PDB created. Now searching abstract state space." << endl;
    pdb_abstraction->compute_goal_distances();
    //pdb_abstraction->dump();
    cout << "Done initializing." << endl;
}

int PDBHeuristic::compute_heuristic(const State &state) {
    return pdb_abstraction->get_heuristic_value(state);
}

ScalarEvaluator *PDBHeuristic::create(const std::vector<string> &config,
                                            int start, int &end, bool dry_run) {
    //TODO: check what we have to do here!
    OptionParser::instance()->set_end_for_simple_config(config, start, end);
    if (dry_run)
        return 0;
    else
        return new PDBHeuristic;
}
