#include "max_evaluator.h"

#include "../plugins/plugin.h"

#include <cassert>

using namespace std;

namespace max_evaluator {
MaxEvaluator::MaxEvaluator(const plugins::Options &opts)
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

class MaxEvaluatorFeature : public plugins::TypedFeature<Evaluator, MaxEvaluator> {
public:
    MaxEvaluatorFeature() : TypedFeature("max") {
        document_subcategory("evaluators_basic");
        document_title("Max evaluator");
        document_synopsis(
            "Calculates the maximum of the sub-evaluators.");
        combining_evaluator::add_combining_evaluator_options_to_feature(*this);
    }

    virtual shared_ptr<MaxEvaluator> create_component(const plugins::Options &options, const plugins::ConstructContext &context) const override {
        context.verify_list_non_empty<shared_ptr<Evaluator>>(options, "evals");
        return make_shared<MaxEvaluator>(options);
    }
};

static plugins::FeaturePlugin<MaxEvaluatorFeature> _plugin;
}
