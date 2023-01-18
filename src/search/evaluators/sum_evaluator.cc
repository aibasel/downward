#include "sum_evaluator.h"

#include "../plugins/plugin.h"

#include <cassert>
#include <limits>

using namespace std;

namespace sum_evaluator {
SumEvaluator::SumEvaluator(const plugins::Options &opts)
    : CombiningEvaluator(opts) {
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

class SumEvaluatorFeature : public plugins::TypedFeature<Evaluator, SumEvaluator> {
public:
    SumEvaluatorFeature() : TypedFeature("sum") {
        document_subcategory("evaluators_basic");
        document_title("Sum evaluator");
        document_synopsis("Calculates the sum of the sub-evaluators.");

        combining_evaluator::add_combining_evaluator_options_to_feature(*this);
    }

    virtual shared_ptr<SumEvaluator> create_component(const plugins::Options &options, const plugins::ConstructContext &context) const override {
        context.verify_list_non_empty<shared_ptr<Evaluator>>(options, "evals");
        return make_shared<SumEvaluator>(options);
    }
};

static plugins::FeaturePlugin<SumEvaluatorFeature> _plugin;
}
