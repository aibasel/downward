#include "ff_synergy.h"

#include "../evaluation_context.h"
#include "../evaluation_result.h"
#include "../option_parser.h"
#include "../plugin.h"

#include <iostream>


namespace landmarks{

using namespace std;

FFSynergyHeuristic::FFSynergyHeuristic(const options::Options &opts)
    : Heuristic(opts)
{}

void FFSynergyHeuristic::set_master(LamaSynergyHeuristic *lama_master){
   master = lama_master;
}

EvaluationResult FFSynergyHeuristic::compute_result(
    EvaluationContext &eval_context){
    /*
       Asking for the master's heuristic value triggers both
       heuristic computations if they have not been computed yet.
       If they have been computed yet, then both heuristic values
       are already cached, and this is just a quick lookup. In
       either case, the result is subsequently available in the
       synergy object.
    */
    eval_context.get_heuristic_value_or_infinity(master);
    return master->ff_result;
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

    Heuristic *ff_synergy = new FFSynergyHeuristic(opts);
    return ff_synergy;
}


// static PluginTypePlugin<FFSynergyHeuristic> _type_plugin(
//     "FFSynergyHeuristic",
//     // TODO: Replace empty string by synopsis for the wiki page.
//     "");

static Plugin<Heuristic> _plugin("ff_syn", _parse);

}
