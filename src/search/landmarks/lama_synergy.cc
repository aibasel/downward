#include "lama_synergy.h"

#include "../evaluation_context.h"
#include "../evaluation_result.h"
#include "../option_parser.h"
#include "../plugin.h"

#include "../lp/lp_solver.h"

using namespace std;

namespace landmarks {
LamaSynergyHeuristic::LamaSynergyHeuristic(const options::Options &opts)
    : Heuristic(opts),
      lama_heuristic(new LandmarkCountHeuristic(opts)) {
    cout << "Initializing LAMA synergy object" << endl;
}

EvaluationResult LamaSynergyHeuristic::compute_result(
    EvaluationContext &eval_context){
    compute_heuristics(eval_context);
    return lama_result;
}

void LamaSynergyHeuristic::compute_heuristics(EvaluationContext &eval_context) {
    /*
      When this method is called, we know that eval_context contains
      results for neither of the two synergy heuristics because the
      method isn't called when a heuristic results is already present,
      and the two results are always added to the evaluation context
      together.
    */

    lama_heuristic->exploration.set_recompute_heuristic();
    lama_result = lama_heuristic->compute_result(eval_context);
    ff_result = lama_heuristic->exploration.compute_result(eval_context);
}


void LamaSynergyHeuristic::notify_initial_state(const GlobalState &initial_state) {
    lama_heuristic->notify_initial_state(initial_state);
}

bool LamaSynergyHeuristic::notify_state_transition(
        const GlobalState &parent_state,
        const GlobalOperator &op,
        const GlobalState &state){
    if (lama_heuristic->notify_state_transition(parent_state, op, state)) {
        if (cache_h_values) {
            heuristic_cache[state].dirty = true;
        }
        return true;
    }
    return false;
}

static Heuristic *_parse(OptionParser &parser) {
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
    parser.add_option<LandmarkFactory *>("lm_factory");
    parser.add_option<bool>("admissible", "get admissible estimate", "false");
    parser.add_option<bool>("optimal", "optimal cost sharing", "false");
    parser.add_option<bool>("alm", "use action landmarks", "true");
    lp::add_lp_solver_option_to_parser(parser);
    Heuristic::add_options_to_parser(parser);

    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;

    /*
      It does not make sense to use the synergy without preferred
      operators, so they are always enabled. (A landmark heuristic
      without preferred operators does not need to perform a relaxed
      exploration, hence no need for a synergy.)
    */
    opts.set("pref", true);

    return new LamaSynergyHeuristic(opts);
}

static Plugin<Heuristic> _plugin("lama_synergy", _parse);
}
