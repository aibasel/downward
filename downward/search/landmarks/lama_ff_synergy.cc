#include "lama_ff_synergy.h"

LamaFFSynergy::HeuristicProxy::HeuristicProxy(LamaFFSynergy *synergy_) {
    synergy = synergy_;
    is_first_proxy = false;
}

void LamaFFSynergy::HeuristicProxy::initialize() {
    if(!synergy->initialized) {
	synergy->initialize();
	is_first_proxy = true;
    }
}

LamaFFSynergy::LamaFFSynergy(bool lm_pref_, bool lm_admissible_, bool lm_optimal_, int lm_type_) :
    lama_heuristic_proxy(this), ff_heuristic_proxy(this),
    lm_pref(lm_pref_), lm_admissible(lm_admissible_), lm_optimal(lm_optimal_), lm_type(lm_type_) {

    cout << "Initializing LAMA-FF Synergy Object" << endl;
    lama_heuristic = new LandmarksCountHeuristic(lm_pref, lm_admissible, lm_optimal, lm_type);
    //lama_heuristic->initialize(); // must be called here explicitly
    exploration = lama_heuristic->get_exploration();
    initialized = false;
}

void LamaFFSynergy::get_lama_preferred_operators(std::vector<const Operator *> &result) {
        result.insert(result.end(),
		  lama_preferred_operators.begin(),
		  lama_preferred_operators.end());
}

void LamaFFSynergy::get_ff_preferred_operators(std::vector<const Operator *> &result) {
        result.insert(result.end(),
		  ff_preferred_operators.begin(),
		  ff_preferred_operators.end());
}

void LamaFFSynergy::compute_heuristics(const State& state) {
    /* Compute heuristics and pref. ops. and store results;
       actual work is delegated to the heuristics. */

    exploration->set_recompute_heuristic();

    lama_heuristic->evaluate(state);
    if(!lama_heuristic->is_dead_end()) {
	lama_heuristic_value = lama_heuristic->get_heuristic();
	lama_preferred_operators.clear();
	lama_heuristic->get_preferred_operators(lama_preferred_operators);
    }
    exploration->evaluate(state);
    if(!exploration->is_dead_end()) {
	ff_heuristic_value = exploration->get_heuristic();
	ff_preferred_operators.clear();
	exploration->get_preferred_operators(ff_preferred_operators);
    }
}

bool LamaFFSynergy::lama_reach_state(const State& parent_state,
        const Operator &op, const State& state) {
    return lama_heuristic->reach_state(parent_state, op, state);
}

