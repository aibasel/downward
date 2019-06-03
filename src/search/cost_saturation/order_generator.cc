#include "order_generator.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../utils/rng_options.h"

using namespace std;

namespace cost_saturation {
OrderGenerator::OrderGenerator(const options::Options &opts)
    : rng(utils::parse_rng_from_options(opts)) {
}

void add_common_order_generator_options(OptionParser &parser) {
    utils::add_rng_options(parser);
}

static PluginTypePlugin<OrderGenerator> _type_plugin(
    "OrderGenerator",
    "Generate heuristic orders.");
}
