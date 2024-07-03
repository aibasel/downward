#include "evaluator.h"

#include "plugins/plugin.h"
#include "utils/logging.h"
#include "utils/system.h"

#include <cassert>

using namespace std;


Evaluator::Evaluator(
    bool use_for_reporting_minima, bool use_for_boosting,
    bool use_for_counting_evaluations, const string &description,
    utils::Verbosity verbosity)
    : description(description),
      use_for_reporting_minima(use_for_reporting_minima),
      use_for_boosting(use_for_boosting),
      use_for_counting_evaluations(use_for_counting_evaluations),
      log(utils::get_log_for_verbosity(verbosity)) {
}

bool Evaluator::dead_ends_are_reliable() const {
    return true;
}

void Evaluator::report_value_for_initial_state(
    const EvaluationResult &result) const {
    if (log.is_at_least_normal()) {
        assert(use_for_reporting_minima);
        log << "Initial heuristic value for " << description << ": ";
        if (result.is_infinite())
            log << "infinity";
        else
            log << result.get_evaluator_value();
        log << endl;
    }
}

void Evaluator::report_new_minimum_value(
    const EvaluationResult &result) const {
    if (log.is_at_least_normal()) {
        assert(use_for_reporting_minima);
        log << "New best heuristic value for " << description << ": "
            << result.get_evaluator_value() << endl;
    }
}

const string &Evaluator::get_description() const {
    return description;
}

bool Evaluator::is_used_for_reporting_minima() const {
    return use_for_reporting_minima;
}

bool Evaluator::is_used_for_boosting() const {
    return use_for_boosting;
}

bool Evaluator::is_used_for_counting_evaluations() const {
    return use_for_counting_evaluations;
}

bool Evaluator::does_cache_estimates() const {
    return false;
}

bool Evaluator::is_estimate_cached(const State &) const {
    return false;
}

int Evaluator::get_cached_estimate(const State &) const {
    ABORT("Called get_cached_estimate when estimate is not cached.");
}

void add_evaluator_options_to_feature(
    plugins::Feature &feature, const string &description) {
    feature.add_option<string>(
        "description",
        "description used to identify evaluator in logs",
        "\"" + description + "\"");
    utils::add_log_options_to_feature(feature);
}

tuple<string, utils::Verbosity> get_evaluator_arguments_from_options(
    const plugins::Options &opts) {
    return tuple_cat(
        make_tuple(opts.get<string>("description")),
        utils::get_log_arguments_from_options(opts)
        );
}

static class EvaluatorCategoryPlugin : public plugins::TypedCategoryPlugin<Evaluator> {
public:
    EvaluatorCategoryPlugin() : TypedCategoryPlugin("Evaluator") {
        document_synopsis(
            "An evaluator specification is either a newly created evaluator "
            "instance or an evaluator that has been defined previously. "
            "This page describes how one can specify a new evaluator instance. "
            "For re-using evaluators, see OptionSyntax#Evaluator_Predefinitions.\n\n"
            "If the evaluator is a heuristic, "
            "definitions of //properties// in the descriptions below:\n\n"
            " * **admissible:** h(s) <= h*(s) for all states s\n"
            " * **consistent:** h(s) <= c(s, s') + h(s') for all states s "
            "connected to states s' by an action with cost c(s, s')\n"
            " * **safe:** h(s) = infinity is only true for states "
            "with h*(s) = infinity\n"
            " * **preferred operators:** this heuristic identifies "
            "preferred operators ");
        allow_variable_binding();
    }
}
_category_plugin;
