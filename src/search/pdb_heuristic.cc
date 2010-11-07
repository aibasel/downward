#include "pdb_heuristic.h"

#include "globals.h"
//#include "option_parser.h"
#include "plugin.h"
#include "state.h"

// AbstractState -----------------------------------------------------------

AbstractState::AbstractState(vector<int> values) {
    variable_values = values;
}

// PDBAbstraction -----------------------------------------------------------

PDBAbstraction::PDBAbstraction(vector<int> p) {
    pattern = p;
    size = 1;
    for (size_t i = 0; i < pattern.size(); i++) {
        size *= g_variable_domain[pattern[i]];
    }
}

void PDBAbstraction::createPDB(PDBHeuristic *pdbheuristic) {
    for (size_t i = 0; i < size; i++) {
        AbstractState *absstate = pdbheuristic->inv_hash_index(i);
        //TODO Operatoren anwenden (zustand erhalten wenn anwendbar), in back-edges eintragen
    }
}


// PDBHeuristic -----------------------------------------------------------

static ScalarEvaluatorPlugin pdb_heuristic_plugin(
    "pdb_heuristic", PDBHeuristic::create);


PDBHeuristic::PDBHeuristic() {
}

PDBHeuristic::~PDBHeuristic() {
}

int PDBHeuristic::hash_index(const AbstractState &absstate) {
    //TODO siehe formeln
    return 0;
}

AbstractState *PDBHeuristic::inv_hash_index(int index) {
    //TODO modulo rechnen
    return 0;
}

void PDBHeuristic::initialize() {
    cout << "Initializing pattern database heuristic..." << endl;
    int patt[5] = {1, 2, 5, 7, 10};
    vector<int> pattern (patt, patt + sizeof(patt) / sizeof(int));
    PDBAbstraction *abstraction = new PDBAbstraction(pattern);
    abstraction->createPDB(this);
}

int PDBHeuristic::compute_heuristic(const State &state) {
    //TODO index berechnen, in tabelle nachschauen
    return 0;
}

ScalarEvaluator *PDBHeuristic::create(const std::vector<string> &config,
                                            int start, int &end, bool dry_run) {
    //OptionParser::instance()->set_end_for_simple_config(config, start, end);
    if (dry_run)
        return 0;
    else
        return new PDBHeuristic;
}
