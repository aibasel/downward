#include "partial_relaxation_heuristic.h"

#include "ff_heuristic.h"
#include "globals.h"
#include "low_level_search.h"
#include "partial_relaxation.h"
#include "state.h"

#include <vector>
using namespace std;

PartialRelaxationHeuristic::PartialRelaxationHeuristic() {
    // The following weird stuff is for Zenotravel.

    /*
    int min_range = 1000;
    for(int var = 0; var < g_variable_domain.size(); var++) {
	int range = g_variable_domain[var];
	if(range < min_range)
	    min_range = range;
    }
    int max_range = 0;
    for(int var = 0; var < g_variable_domain.size(); var++) {
	int range = g_variable_domain[var];
	if(range > max_range)
	    max_range = range;
    }
    int median_range = min_range;
    for(int var = 0; var < g_variable_domain.size(); var++) {
	int range = g_variable_domain[var];
	if(range != min_range && range != max_range)
	    median_range = range;
    }
    */
    // TODO: Build relaxation in some reasonable way.
    vector<int> relaxed_variables;
    for(int var = 0; var < g_variable_domain.size(); var++) {
//	int range = g_variable_domain[var];
//	if(range != 26 && range != 37 && range != 50
//	   && range != 65 && range != 82) // HACK HACK HACK
	if(var != 0)
//	if(range != median_range)
	    relaxed_variables.push_back(var);
    }
    relaxation = new PartialRelaxation(relaxed_variables);
    low_level_search_engine = new LowLevelSearchEngine(*relaxation);
    low_level_heuristic = new FFHeuristic(true);
    // low_level_heuristic = new FFHeuristic(false);
    low_level_search_engine->add_heuristic(low_level_heuristic, true, true);
}

PartialRelaxationHeuristic::~PartialRelaxationHeuristic() {
    delete low_level_search_engine;
    delete low_level_heuristic;
    delete relaxation;
}

void PartialRelaxationHeuristic::initialize() {
    cout << "Initializing partial relaxation heuristic..." << endl;
}

int PartialRelaxationHeuristic::compute_heuristic(const State &state) {
    int result = low_level_search_engine->search(state);
    vector<const Operator *> preferred;
    low_level_search_engine->get_preferred_operators(preferred);
    for(int i = 0; i < preferred.size(); i++)
	set_preferred(preferred[i]);
    return result;
}

