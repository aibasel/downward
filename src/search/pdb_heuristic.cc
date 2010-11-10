#include "pdb_heuristic.h"

#include "globals.h"
//#include "option_parser.h"
#include "plugin.h"
#include "state.h"
#include "operator.h"
#include <cassert>
#include <stdlib.h>
#include <queue>

// AbstractState --------------------------------------------------------------

AbstractState::AbstractState(vector<int> var_vals, vector<int> pattern) {
    variable_values = var_vals;
    for (size_t i = 0; i < pattern.size(); i++) {
        variale_to_index_mapping[pattern[i]] = i;
        index_to_variable_mapping[i] = pattern[i];
    }
}

AbstractState::AbstractState(const State &state, vector<int> pattern) {
    variable_values = vector<int>(pattern.size()); // at position i, value of variable from pattern[i] is stored
    for (size_t i = 0; i < pattern.size(); i++) {
        int var = pattern[i];
        int val = state[var];
        variale_to_index_mapping[var] = i;
        index_to_variable_mapping[i] = pattern[i];
        variable_values[i] = val;
    }
}

vector<int> AbstractState::get_variable_values() const {
    return variable_values;
}

void AbstractState::dump() const {
    cout << "AbstractState: " << endl;
    for (size_t i = 0; i < variable_values.size(); i++) {
        map<int, int> index_map = index_to_variable_mapping; //WHY THE HELL IS THIS REQUIRED?!?!?!?!?!
        int variable = index_map[i]; //WHY THE HELL IS THIS REQUIRED?!?!?!?!?!
        int value = variable_values[i];
        cout << "Index: " << i << " Variable: " <<  variable << " Value: " << value << endl;
    }
}

// AbstractOperator -----------------------------------------------------------

AbstractOperator::AbstractOperator(Operator &o, vector<int> pat) {
    pattern = pat;
    const vector<PrePost> pre_post = o.get_pre_post();
    pre_effect = vector<PreEffect>();
    for (size_t i = 0; i < pre_post.size(); i++) {
        int var = pre_post[i].var;
        for (size_t j = 0; j < pattern.size(); j++) {
            if (var == pattern[j]) {
                int pre = pre_post[i].pre;
                int post = pre_post[i].post;
                pre_effect.push_back(PreEffect(var, pair<int, int>(pre, post)));
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
        map<int, int> index_map = abstract_state.variale_to_index_mapping;
        if (!(pre == -1 || abstract_state.get_variable_values()[index_map[var]] == pre))
            return false;
    }
    return true;
}

const AbstractState AbstractOperator::apply_operator(const AbstractState &abstract_state) {
    assert(is_applicable(abstract_state));
    vector<int> variable_values = abstract_state.get_variable_values();
    for (size_t i = 0; i < pre_effect.size(); i++) {
        int var = pre_effect[i].first;
        int pre = pre_effect[i].second.first;
        int post = pre_effect[i].second.second;
        map<int, int> index_map = abstract_state.variale_to_index_mapping;
        assert(variable_values[index_map[var]] == pre || pre == -1);
        variable_values[index_map[var]] = post;
    }
    return AbstractState(variable_values, pattern);
}

void AbstractOperator::dump() const {
    cout << "AbstractOperator: " << endl;
    for (size_t i = 0; i < pre_effect.size(); i++) {
        cout << "Variable: " << pre_effect[i].first << " (True name: " << g_variable_name[pre_effect[i].first] << ") Pre: " << pre_effect[i].second.first
            << " Post: "<< pre_effect[i].second.second << endl;
    }
}

// PDBAbstraction -------------------------------------------------------------

PDBAbstraction::PDBAbstraction(vector<int> pat) {
    pattern = pat;
    size = 1; // number of abstract states
    for (size_t i = 0; i < pattern.size(); i++) {
        size *= g_variable_domain[pattern[i]];
    }
    distances = vector<int>(size);
    back_edges = vector<vector<Edge > >(size);

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
    //cout << "Calculated n_i. Now abstracting operators." << endl;
    vector<AbstractOperator *> operators(g_operators.size());
    for (size_t i = 0; i < g_operators.size(); i++) {
        operators[i] = new AbstractOperator(g_operators[i], pattern);
    }
    //cout << "Calculated abstracted operators. Now computing the abstract space (graph)." << endl;
    for (size_t i = 0; i < size; i++) {
        //cout << "Iteration No.: " << i << endl;
        const AbstractState *abstract_state = inv_hash_index(i);
        //abstract_state->dump();
        //cout << endl;
        for (size_t j = 0; j < operators.size(); j++){
            //cout << "Operator No.: " << j << endl;
            //operators[j]->dump();
            if (operators[j]->is_applicable(*abstract_state)) {
                //cout << "operator applicable" << endl;
                const AbstractState next_state = operators[j]->apply_operator(*abstract_state);
                //cout << "next state: " << endl;
                //next_state.dump();
                int state_index = hash_index(next_state);
                back_edges[state_index].push_back(Edge(&(g_operators[j]), i));
            }
            //cout << endl;
        }
        //cout << endl;
    }
    //cout << "Graph setup complete." << endl;
}

void PDBAbstraction::compute_goal_distances() {
    // backward search from abstract goal state to all abstract states
    /*int goal_index = hash_index(AbstractState(g_goal,pattern));
    queue<pair<int,int>> que;
    que.push(pair<int,int>(goal_index,0));
    distances[goal_index] = 0;
    while (!que.empty) {
        // get edges from actual abstract state and add them to queue
        pair<int,int> actual = que.pop();
        vector<Edge> vec = back_edges[actual.first];
        for (t_size i = 0;i < vec.size();i++) {
            actual_distance = actual.second + vec[i].op->get_cost();
            distances[vec[i]] = actual_distance;
            que.push(pair<int,int>(vec[i].target,actual_distance));
        }
    }*/
}

int PDBAbstraction::hash_index(const AbstractState &abstract_state) {
    int index = 0;
    for (int i = 0;i < pattern.size();i++) {
        index += n_i[i]*abstract_state.get_variable_values()[i];
    }
    return index;
}

const AbstractState *PDBAbstraction::inv_hash_index(int index) {
    vector<int> var_vals = vector<int>(pattern.size());
    for (int n = 1;n < pattern.size();n++) {
        int d = index%n_i[n];
        var_vals[n-1] = (int) d/n_i[n-1];
        index -= d;
    }
    var_vals[pattern.size()-1] = (int) index/n_i[pattern.size()-1];
    return new AbstractState(var_vals, pattern);
}

int PDBAbstraction::get_heuristic_value(const State &state) {
    const AbstractState abstract_state(state, pattern);
    int index = hash_index(abstract_state);
    return distances[index];
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
    int patt[2] = {1, 2};
    vector<int> pattern(patt, patt + sizeof(patt) / sizeof(int));
    //cout << "Try creating a new PDBAbstraction" << endl << endl;
    pdb_abstraction = new PDBAbstraction(pattern);
    //cout << "Created new PDBAbstraction. Now calling create_pdb()" << endl << endl;
    pdb_abstraction->create_pdb();
    //cout << "PDB created. Now searching abstract state space." << endl;
    pdb_abstraction->compute_goal_distances();
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
