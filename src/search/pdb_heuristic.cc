#include "pdb_heuristic.h"

#include "globals.h"
//#include "option_parser.h"
#include "plugin.h"
#include "state.h"
#include "operator.h"

// AbstractState --------------------------------------------------------------

AbstractState::AbstractState(vector<int> values) {
    variable_values = values;
}

AbstractState::AbstractState(const State &state) {
    //TODO
}

// AbstractOperator -----------------------------------------------------------

AbstractOperator::AbstractOperator(Operator op, vector<int> p) {
    //TODO
}

bool AbstractOperator::is_applicable(AbstractState &abstract_state) {
    //TODO
    return true;
}

const AbstractState AbstractOperator::apply_operator(AbstractState &abstract_state) {
    //TODO
    vector<int> values;
    return AbstractState(values);
}

// PDBAbstraction -------------------------------------------------------------

PDBAbstraction::PDBAbstraction(vector<int> p) {
    pattern = p;
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
        AbstractState *abstract_state = inv_hash_index(i);
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

AbstractState *PDBAbstraction::inv_hash_index(int index) {
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

void PDBHeuristic::initialize() {
    cout << "Initializing pattern database heuristic..." << endl;
    int patt[5] = {1, 2, 5, 7, 10};
    vector<int> pattern (patt, patt + sizeof(patt) / sizeof(int));
    pdb_abstraction = new PDBAbstraction(pattern);
    pdb_abstraction->create_pdb();
}

int PDBHeuristic::compute_heuristic(const State &state) {
    const AbstractState abstract_state(state);
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
