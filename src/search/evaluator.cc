#include "evaluator.h"

#include "option_parser.h"
#include "plugin.h"

#include "utils/logging.h"
#include "utils/system.h"

#include <cassert>

using namespace std;


Evaluator::Evaluator(const string &description,
                     bool use_for_reporting_minima,
                     bool use_for_boosting,
                     bool use_for_counting_evaluations)
    : description(description),
      use_for_reporting_minima(use_for_reporting_minima),
      use_for_boosting(use_for_boosting),
      use_for_counting_evaluations(use_for_counting_evaluations) {
}

bool Evaluator::dead_ends_are_reliable() const {
    return true;
}

void Evaluator::report_value_for_initial_state(const EvaluationResult &result) const {
    assert(use_for_reporting_minima);
    utils::g_log << "Initial heuristic value for " << description << ": ";
    if (result.is_infinite())
        utils::g_log << "infinity";
    else
        utils::g_log << result.get_evaluator_value();
    utils::g_log << endl;
}

void Evaluator::report_new_minimum_value(const EvaluationResult &result) const {
    assert(use_for_reporting_minima);
    utils::g_log << "New best heuristic value for " << description << ": "
                 << result.get_evaluator_value() << endl;
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

bool Evaluator::is_estimate_cached(const GlobalState &) const {
    return false;
}

int Evaluator::get_cached_estimate(const GlobalState &) const {
    ABORT("Called get_cached_estimate when estimate is not cached.");
}

static PluginTypePlugin<Evaluator> _type_plugin(
    "Evaluator",
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
    "preferred operators ",
    "evaluator", "heuristic");
