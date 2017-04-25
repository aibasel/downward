#include "ff_synergy.h"

#include "../evaluation_context.h"
#include "../evaluation_result.h"
#include "../option_parser.h"
#include "../plugin.h"


namespace landmarks{

FFSynergyHeuristic::FFSynergyHeuristic(const options::Options &opts)
    : Heuristic(opts){}


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

}
