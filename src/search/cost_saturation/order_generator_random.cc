#include "order_generator_random.h"

#include "utils.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../utils/logging.h"
#include "../utils/rng.h"
#include "../utils/rng_options.h"

using namespace std;

namespace cost_saturation {
OrderGeneratorRandom::OrderGeneratorRandom(const Options &opts)
    : rng(utils::parse_rng_from_options(opts)) {
}

void OrderGeneratorRandom::initialize(
    const Abstractions &abstractions,
    const vector<int> &) {
    utils::Log() << "Initialize random order generator" << endl;
    random_order = get_default_order(abstractions.size());
}

Order OrderGeneratorRandom::compute_order_for_state(
    const Abstractions &,
    const vector<int> &,
    const vector<int> &,
    bool) {
    rng->shuffle(random_order);
    return random_order;
}


static shared_ptr<OrderGenerator> _parse_greedy(OptionParser &parser) {
    utils::add_rng_options(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<OrderGeneratorRandom>(opts);
}

static Plugin<OrderGenerator> _plugin_greedy("random_orders", _parse_greedy);
}
