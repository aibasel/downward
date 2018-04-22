#include "saturated_cost_partitioning_heuristic.h"

#include "cost_partitioned_heuristic.h"
#include "utils.h"

#include "../option_parser.h"
#include "../plugin.h"

using namespace std;

namespace cost_saturation {
static Heuristic *_parse(OptionParser &parser) {
    parser.document_synopsis(
        "Saturated cost partitioning heuristic",
        "");
    return get_max_cp_heuristic(parser, compute_saturated_cost_partitioning);
}

static Plugin<Heuristic> _plugin("saturated_cost_partitioning", _parse);
}
