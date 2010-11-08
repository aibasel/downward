#include "pdb_heuristic.h"

#include "globals.h"
//#include "option_parser.h"
#include "plugin.h"
#include "state.h"
#include "operator.h"
#include <cassert>
#include <stdlib.h>

// AbstractState --------------------------------------------------------------

AbstractState::AbstractState(vector<int> var_vals) {
    variable_values = var_vals;
}

AbstractState::AbstractState(const State &state, vector<int> pattern) {
    variable_values = vector<int>(g_variable_name.size(), -10); 
        // -10 is the default value for variables not included in the abstraction
    for (size_t i = 0; i < pattern.size(); i++) {
        int var = pattern[i];
        int val = state[var];
        variable_values[var] = val;
    }
}

vector<int> AbstractState::get_variable_values() const {
    return variable_values;
}

// AbstractOperator -----------------------------------------------------------

AbstractOperator::AbstractOperator(Operator &o, vector<int> pattern) {
    name = o.get_name();
    cost = o.get_cost();
    const vector<PrePost> pre_post = o.get_pre_post();
    pre_effect = vector<PreEffect>(pre_post.size());
    for (size_t i = 0; i < pre_post.size(); i++) {
        int var = pre_post[i].var;
        int pre = pre_post[i].pre;
        int post = pre_post[i].post;
        pre_effect[i].first = var;
        pre_effect[i].second = pair<int, int>(-10, -10);
        for (size_t j = 0; j < pattern.size(); j++) {
            if (var == pattern[j]) {
                pre_effect[i].second = pair<int, int>(pre, post);
                break;
            }
        }
    }
}

bool AbstractOperator::is_applicable(const AbstractState &abstract_state) const {
    for (int i = 0; i < pre_effect.size(); i++) {
        int var = pre_effect[i].first;
        int pre = pre_effect[i].second.first;
        assert(var >= 0 && var < g_variable_name.size());
        assert(pre == -1 || pre == -10 || (pre >= 0 && pre < g_variable_domain[var]));
        if (!(pre == -1 || pre == -10 || abstract_state.get_variable_values()[var] == pre))
            return false;
    }
    return true;
}

const AbstractState AbstractOperator::apply_operator(const AbstractState &abstract_state) {
    assert(is_applicable(abstract_state));
    vector<int> variable_values = abstract_state.get_variable_values();
    for (int i = 0; i < pre_effect.size(); i++) {
        int var = pre_effect[i].first;
        int pre = pre_effect[i].second.first;
        int post = pre_effect[i].second.second;
        assert(variable_values[var] == pre); //check!
        if (variable_values[var] != pre)
            exit(1); //pre otherwise counts as non-used variable... whatever.
        variable_values[var] = post;
    }
    return AbstractState(variable_values);
}

// PDBAbstraction -------------------------------------------------------------

PDBAbstraction::PDBAbstraction(vector<int> pat) {
    pattern = pat;
    size = 1;
    for (size_t i = 0; i < pattern.size(); i++) {
        size *= g_variable_domain[pattern[i]];
    }
    distances = vector<int>(size);
    back_edges = vector<vector<Edge > >(size);
    n_i = vector<int>(size);
    for (int i = 0; i < size; i++) {
        int j = 0;
        int p = 1;
        for (j = 0; j < i; j++) {
            p *= g_variable_domain[pattern[j]] + 1;
        }
        n_i[i] = p;
    }
}

void PDBAbstraction::create_pdb() {
    vector<AbstractOperator *> operators(g_operators.size());
    for (size_t i = 0; i < g_operators.size(); i++) {
        operators[i] = new AbstractOperator(g_operators[i], pattern);
    }
    for (size_t i = 0; i < size; i++) {
        const AbstractState *abstract_state = inv_hash_index(i);
        for (size_t j = 0; j < operators.size(); j++){
            if (operators[j]->is_applicable(*abstract_state)) {
                const AbstractState next_state = operators[j]->apply_operator(*abstract_state);
                int state_index = hash_index(next_state);
                back_edges[state_index].push_back(Edge(&(g_operators[j]), i));
            }
        }
    }
}

int PDBAbstraction::hash_index(const AbstractState &abstract_state) {
    //TODO siehe formeln
    return 0;
}

const AbstractState *PDBAbstraction::inv_hash_index(int index) {
    //TODO modulo rechnen
    return 0;
}

int PDBAbstraction::get_goal_distance(const AbstractState &abstract_state) {
    int index = hash_index(abstract_state);
    return distances[index];
}

// PDBHeuristic ---------------------------------------------------------------

static ScalarEvaluatorPlugin pdb_heuristic_plugin(
    "pdb_heuristic", PDBHeuristic::create);

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
    cout << "Initializing pattern database heuristic..." << endl;
    verify_no_axioms_no_cond_effects();
    int patt[2] = {1, 2};
    pattern = vector<int>(patt, patt + sizeof(patt) / sizeof(int));
    pdb_abstraction = new PDBAbstraction(pattern);
    pdb_abstraction->create_pdb();
}

int PDBHeuristic::compute_heuristic(const State &state) {
    const AbstractState abstract_state(state, pattern);
    return pdb_abstraction->get_goal_distance(abstract_state);
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
