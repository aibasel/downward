#include "ff_synergy.h"

#include "../evaluation_context.h"
#include "../evaluation_result.h"
#include "../option_parser.h"
#include "../plugin.h"

#include "../utils/system.h"

using namespace std;
using utils::ExitCode;

namespace landmarks{
FFSynergyHeuristic::FFSynergyHeuristic(const options::Options &opts)
    : Heuristic(opts),
      master(dynamic_cast<LamaSynergyHeuristic *>(
                  opts.get<Heuristic *>("lama_synergy_heuristic")))
{
    if (!master) {
        cerr << "ff_synergy requires a lama_synergy heuristic" << endl;
        utils::exit_with(ExitCode::INPUT_ERROR);
    }
}

EvaluationResult FFSynergyHeuristic::compute_result(
    EvaluationContext &eval_context){
    /*
       Asking for the master's heuristic value triggers both
       heuristic computations if they have not been computed yet.
       If they have been computed yet, then both heuristic values
       are already cached, and this is just a quick lookup. In
       either case, the result is subsequently available in the
       master object.
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
    parser.add_option<Heuristic *>("lama_synergy_heuristic");

    // Note that we deliberately omit options from the Heuristic base class
    // since they are ignored anyway.
    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;

    return new FFSynergyHeuristic(opts);
}

static Plugin<Heuristic> _plugin("ff_synergy", _parse);

}
