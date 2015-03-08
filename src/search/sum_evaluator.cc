#include "sum_evaluator.h"

#include <cassert>
#include <limits>

#include "option_parser.h"
#include "plugin.h"

SumEvaluator::SumEvaluator(const Options &opts)
    : CombiningEvaluator(opts.get_list<ScalarEvaluator *>("evals")) {
}

SumEvaluator::SumEvaluator(const std::vector<ScalarEvaluator *> &evals)
    : CombiningEvaluator(evals) {
}

SumEvaluator::~SumEvaluator() {
}

int SumEvaluator::combine_values(const vector<int> &values) {
    int result = 0;
    for (int value : values) {
        assert(value >= 0);
        result += value;
        assert(result >= 0); // Check against overflow.
    }
    return result;
}

static ScalarEvaluator *_parse(OptionParser &parser) {
    parser.document_synopsis("Sum evaluator",
                             "Calculates the sum of the sub-evaluators.");

    parser.add_list_option<ScalarEvaluator *>("evals",
                                              "at least one scalar evaluator");
    Options opts = parser.parse();

    opts.verify_list_non_empty<ScalarEvaluator *>("evals");

    if (parser.dry_run())
        return 0;
    else
        return new SumEvaluator(opts);
}

static Plugin<ScalarEvaluator> _plugin("sum", _parse);
