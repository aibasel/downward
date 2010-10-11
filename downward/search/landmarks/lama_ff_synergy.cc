#include "lama_ff_synergy.h"
#include "../option_parser.h"
#include "../plugin.h"


static SynergyPlugin lama_ff_synergy_plugin(
    "lm_ff_syn", LamaFFSynergy::create_heuristics);


LamaFFSynergy::HeuristicProxy::HeuristicProxy(LamaFFSynergy *synergy_) {
    synergy = synergy_;
    is_first_proxy = false;
}

void LamaFFSynergy::HeuristicProxy::initialize() {
    if (!synergy->initialized) {
        synergy->initialize();
        is_first_proxy = true;
    }
}

LamaFFSynergy::LamaFFSynergy(bool lm_pref_, bool lm_admissible_, bool lm_optimal_, int lm_type_)
    : lama_heuristic_proxy(this), ff_heuristic_proxy(this),
      lm_pref(lm_pref_), lm_admissible(lm_admissible_), lm_optimal(lm_optimal_), lm_type(lm_type_) {
    cout << "Initializing LAMA-FF Synergy Object" << endl;
    lama_heuristic = new LandmarkCountHeuristic(lm_pref, lm_admissible, lm_optimal, lm_type);
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

void LamaFFSynergy::compute_heuristics(const State &state) {
    /* Compute heuristics and pref. ops. and store results;
       actual work is delegated to the heuristics. */

    exploration->set_recompute_heuristic();

    lama_heuristic->evaluate(state);
    if (!lama_heuristic->is_dead_end()) {
        lama_heuristic_value = lama_heuristic->get_heuristic();
        lama_preferred_operators.clear();
        lama_heuristic->get_preferred_operators(lama_preferred_operators);
    }
    exploration->evaluate(state);
    if (!exploration->is_dead_end()) {
        ff_heuristic_value = exploration->get_heuristic();
        ff_preferred_operators.clear();
        exploration->get_preferred_operators(ff_preferred_operators);
    }
}

bool LamaFFSynergy::lama_reach_state(const State &parent_state,
                                     const Operator &op, const State &state) {
    return lama_heuristic->reach_state(parent_state, op, state);
}

void
LamaFFSynergy::create_heuristics(const std::vector<string> &config,
                                 int start, int &end,
                                 vector<Heuristic *> &heuristics) {
    int lm_type_ = LandmarkCountHeuristic::rpg_sasp;
    bool lm_admissible_ = false;
    bool lm_optimal_ = false;

    // "<name>()" or "<name>(<options>)"
    if (config.size() > start + 2 && config[start + 1] == "(") {
        end = start + 2;

        if (config[end] != ")") {
            NamedOptionParser option_parser;
            option_parser.add_int_option("lm_type",
                                         &lm_type_,
                                         "landmarks type");
            option_parser.add_bool_option("optimal",
                                          &lm_optimal_,
                                          "optimal cost sharing");
            option_parser.add_bool_option("admissible",
                                          &lm_admissible_,
                                          "get admissible estimate");
            option_parser.parse_options(config, end, end, false);
            ++end;
        }
        if (config[end] != ")")
            throw ParseError(end);
    } else { // "<name>"
        end = start;
    }

    bool lm_pref_ = true; // this will always be the case because it
                          // does not make sense to use the synergy without
                          // using lm preferred operators
    LamaFFSynergy *lama_ff_synergy = new LamaFFSynergy(
        lm_pref_, lm_admissible_, lm_optimal_, lm_type_);

    heuristics.push_back(lama_ff_synergy->get_lama_heuristic_proxy());
    heuristics.push_back(lama_ff_synergy->get_ff_heuristic_proxy());
    return;
}
