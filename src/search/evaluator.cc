#include "evaluator.h"

#include "plugin.h"

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
    cout << "Initial heuristic value for " << description << ": ";
    if (result.is_infinite())
        cout << "infinity";
    else
        cout << result.get_h_value();
    cout << endl;
}

void Evaluator::report_new_minimum_value(const EvaluationResult &result) const {
    assert(use_for_reporting_minima);
    cout << "New best heuristic value for " << description << ": "
         << result.get_h_value() << endl;
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


static PluginTypePlugin<Evaluator> _type_plugin(
    "Evaluator",
    // TODO: Replace empty string by synopsis for the wiki page.
    "");
