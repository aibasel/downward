#include "lama_ff_synergy.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "landmarks_graph_rpg_sasp.h"


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

LamaFFSynergy::LamaFFSynergy(const Options &opts)
    : lama_heuristic_proxy(this), ff_heuristic_proxy(this),
      lm_pref(opts.get<bool>("pref")), 
      lm_admissible(opts.get<bool>("admissible")),  
      lm_optimal(opts.get<bool>("optimal")), 
      use_action_landmarks(opts.get<bool>("alm"))
{
    cout << "Initializing LAMA-FF Synergy Object" << endl;
    lama_heuristic = 
        new LandmarkCountHeuristic(opts);
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

static Synergy *_parse_heuristics(OptionParser& parser) {
    Synergy *syn = new Synergy; 
    parser.add_option<LandmarksGraph *>("lm_graph");
    parser.add_option<bool>("admissible", false, "get admissible estimate");
    parser.add_option<bool>("optimal", false, "optimal cost sharing");
    parser.add_option<bool>("alm", true, "use action landmarks");
    
    Options opts = parser.parse();
    if(parser.help_mode())
        return 0;
    bool lm_pref_ = true; // this will always be the case because it
                          // does not make sense to use the synergy without
                          // using lm preferred operators
    opts.set("pref", lm_pref_);

    if(!parser.dry_run()){
        LamaFFSynergy *lama_ff_synergy = 
            new LamaFFSynergy(opts);

        syn->heuristics.push_back(lama_ff_synergy->get_lama_heuristic_proxy());
        syn->heuristics.push_back(lama_ff_synergy->get_ff_heuristic_proxy());
    } else {
        syn->heuristics.push_back(0);
        syn->heuristics.push_back(0);
    }
    return syn;
}

static SynergyPlugin lama_ff_synergy_plugin("lm_ff_syn", _parse_heuristics);
