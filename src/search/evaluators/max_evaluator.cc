#include "max_evaluator.h"

#include "../option_parser.h"
#include "../plugin.h"

#include <cassert>

using namespace std;

namespace max_evaluator {
MaxEvaluator::MaxEvaluator(const Options &opts)
    : CombiningEvaluator(opts) {
}

MaxEvaluator::~MaxEvaluator() {
}

int MaxEvaluator::combine_values(const vector<int> &values) {
    int result = 0;
    for (int value : values) {
        assert(value >= 0);
        result = max(result, value);
    }
    return result;
}

static shared_ptr<Evaluator> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "Max evaluator",
        "Calculates the maximum of the sub-evaluators.");
    combining_evaluator::add_combining_evaluator_options_to_parser(parser);

    Options opts = parser.parse();

    opts.verify_list_non_empty<shared_ptr<Evaluator>>("evals");

    if (parser.dry_run()) {
        return nullptr;
    }
    return make_shared<MaxEvaluator>(opts);
}

static Plugin<Evaluator> plugin("max", _parse, "evaluators_basic");
}
