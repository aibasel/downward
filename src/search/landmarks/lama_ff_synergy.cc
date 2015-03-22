#include "lama_ff_synergy.h"

#include "landmark_factory_rpg_sasp.h"

#include "../evaluation_context.h"
#include "../evaluation_result.h"
#include "../option_parser.h"
#include "../plugin.h"

using namespace std;


LamaFFSynergy::LamaFFSynergy(const Options &opts)
    : lama_master_heuristic(new LamaMasterHeuristic(this)),
      ff_slave_heuristic(new FFSlaveHeuristic(this, lama_master_heuristic.get())),
      lm_pref(opts.get<bool>("pref")),
      lm_admissible(opts.get<bool>("admissible")),
      lm_optimal(opts.get<bool>("optimal")),
      use_action_landmarks(opts.get<bool>("alm")) {
    cout << "Initializing LAMA-FF Synergy Object" << endl;
    lama_heuristic = new LandmarkCountHeuristic(opts);
    exploration = lama_heuristic->get_exploration();
}

void LamaFFSynergy::compute_heuristics(EvaluationContext &eval_context) {
    /*
      When this method is called, we know that eval_context contains
      results for neither of the two synergy heuristics because the
      method isn't called when a heuristic results is already present,
      and the two results are always added to the evaluation context
      together.
    */

    exploration->set_recompute_heuristic();
    lama_result = lama_heuristic->compute_result(eval_context);
    ff_result = exploration->compute_result(eval_context);
}

bool LamaFFSynergy::lama_reach_state(
    const GlobalState &parent_state, const GlobalOperator &op,
    const GlobalState &state) {
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

    LamaFFSynergy *lama_ff_synergy = new LamaFFSynergy(opts);
    Synergy *syn = new Synergy;
    syn->heuristics.push_back(lama_ff_synergy->get_lama_heuristic_proxy());
    syn->heuristics.push_back(lama_ff_synergy->get_ff_heuristic_proxy());
    return syn;
}

static Plugin<Synergy> _plugin("lm_ff_syn", _parse);
