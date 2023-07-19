#include "sum_evaluator.h"

#include "../plugins/plugin.h"

#include <cassert>

using namespace std;

namespace sum_evaluator {
SumEvaluator::SumEvaluator(const plugins::Options &opts)
    : CombiningEvaluator(opts) {
}

SumEvaluator::SumEvaluator(basic_string<char> unparsed_config,
                           bool use_for_reporting_minima,
                           bool use_for_boosting,
                           bool use_for_counting_evaluations,
                           utils::LogProxy log,
                           vector<shared_ptr<Evaluator>> subevaluators)
            : CombiningEvaluator(unparsed_config, use_for_reporting_minima, use_for_boosting, use_for_counting_evaluations, log, subevaluators) {
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

    virtual shared_ptr<SumEvaluator> create_component(
            const plugins::Options &opts, const utils::Context &context) const override {
        plugins::verify_list_non_empty<shared_ptr<Evaluator>>(context, opts, "evals");
        return make_shared<SumEvaluator>(opts.get_unparsed_config(),
                                       opts.get<bool>("use_for_reporting_minima"),
                                       opts.get<bool>("use_for_boosting"),
                                       opts.get<bool>("use_for_counting_evaluations"),
                                       utils::get_log_from_options(opts),
                                       opts.get_list<shared_ptr<Evaluator>>("evals")
        );
    }
};

static plugins::FeaturePlugin<SumEvaluatorFeature> _plugin;
}
