#include "pdb_heuristic.h"

#include "globals.h"
#include "option_parser.h"
#include "plugin.h"
#include "state.h"


// PDBAbstraction -----------------------------------------------------------

PDBAbstraction::PDBAbstraction(vector<int> pattern) {
    this.pattern = pattern;
    size = 1;
    for (size_t i = 0; i < pattern.size(); i++) {
        size *= g_variable_domain[pattern[i]];
    }
}

PDBAbstraction::createPDB() {
    for (size_t i = 0; i < size; i++) {
        AbstractState *absstate = inv_hash_index(i);
        //TODO Operatoren anwenden (zustand erhalten wenn anwendbar), in back-edges eintragen
    }
}


// PDBHeuristic -----------------------------------------------------------

static ScalarEvaluatorPlugin pdb_heuristic_plugin(
    "goalcount", PDBHeuristic::create);


PDBHeuristic::PDBHeuristic() {
}

PDBHeuristic::~PDBHeuristic() {
}

int PDBHEeuristic::hash_index(const AbstractState &state) {
    //TODO siehe formeln
}

*AbstracState PDBHEeuristic::inv_hash_index(int index) {
    //TODO modulorechnen
}

void PDBHeuristic::initialize() {
    cout << "Initializing pattern database heuristic..." << endl;
    int patt[5] = {1, 2, 5, 7, 10};
    vector<int> pattern (patt, patt + sizeof(patt) / sizeof(int));
    PDBAbstraction *abstraction = new PDBAbstraction(pattern);
    abstraction->createPDB();
}

int PDBHeuristic::compute_heuristic(const State &state) {
    //TODO index berechnen, in tabelle nachschauen
}

ScalarEvaluator *PDBHeuristic::create(const std::vector<string> &config,
                                            int start, int &end, bool dry_run) {
    OptionParser::instance()->set_end_for_simple_config(config, start, end);
    if (dry_run)
        return 0;
    else
        return new PDBHeuristic;
}
