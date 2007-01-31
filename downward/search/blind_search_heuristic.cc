#include "blind_search_heuristic.h"

#include "globals.h"
#include "state.h"

BlindSearchHeuristic::BlindSearchHeuristic() {
}

BlindSearchHeuristic::~BlindSearchHeuristic() {
}

void BlindSearchHeuristic::initialize() {
    cout << "Initializing blind search heuristic..." << endl;
}

int BlindSearchHeuristic::compute_heuristic(const State &state) {
    for(int i = 0; i < g_goal.size(); i++) {
	int var = g_goal[i].first, value = g_goal[i].second;
	if(state[var] != value)
            return 1;
    }
    return 0;
}
