#include "lama_ff_synergy.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "landmark_factory_rpg_sasp.h"


LamaFFSynergy::HeuristicProxy::HeuristicProxy(LamaFFSynergy *synergy_)
    : Heuristic(Heuristic::default_options()) {
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
      use_action_landmarks(opts.get<bool>("alm")) {
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
    lama_preferred_operators.clear();
    ff_preferred_operators.clear();

    lama_heuristic->evaluate(state);
    if (!lama_heuristic->is_dead_end()) {
        lama_heuristic_value = lama_heuristic->get_heuristic();
        lama_heuristic->get_preferred_operators(lama_preferred_operators);
    } else {
        lama_heuristic_value = -1;
    }

    exploration->evaluate(state);
    if (!exploration->is_dead_end()) {
        ff_heuristic_value = exploration->get_heuristic();
        exploration->get_preferred_operators(ff_preferred_operators);
    } else {
        ff_heuristic_value = -1;
    }
}

bool LamaFFSynergy::lama_reach_state(const State &parent_state,
                                     const Operator &op, const State &state) {
    return lama_heuristic->reach_state(parent_state, op, state);
}

static Synergy *_parse(OptionParser &parser) {
    parser.document_synopsis(
        "LAMA-FF synergy",
        "If the FF heuristic should be used "
        "(for its estimates or its preferred operators) "
        "and we want to use preferred operators of the "
        "landmark count heuristic, we can exploit synergy effects by "
        "using the LAMA-FF synergy. "
        "This synergy can only be used via Predefinition "
        "(see OptionSyntax#Predefinitions), for example:\n"
        "\"hlm,hff=lm_ff_syn(...)\"");
    parser.add_option<LandmarkGraph *>("lm_graph");
    parser.add_option<bool>("admissible", "get admissible estimate", "false");
    parser.add_option<bool>("optimal", "optimal cost sharing", "false");
    parser.add_option<bool>("alm", "use action landmarks", "true");
    Heuristic::add_options_to_parser(parser);

    Options opts = parser.parse();
    if (parser.dry_run())
        return 0;

    bool lm_pref_ = true; // this will always be the case because it
                          // does not make sense to use the synergy without
                          // using lm preferred operators
    opts.set("pref", lm_pref_);

    LamaFFSynergy *lama_ff_synergy =
        new LamaFFSynergy(opts);
    Synergy *syn = new Synergy;
    syn->heuristics.push_back(lama_ff_synergy->get_lama_heuristic_proxy());
    syn->heuristics.push_back(lama_ff_synergy->get_ff_heuristic_proxy());
    return syn;
}

static Plugin<Synergy> _plugin("lm_ff_syn", _parse);
