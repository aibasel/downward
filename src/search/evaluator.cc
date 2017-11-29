#include "evaluator.h"

#include "plugin.h"

using namespace std;


Evaluator::Evaluator(const string &description, bool enable_statistics)
    : description(description),
      enable_statistics(enable_statistics) {
}

bool Evaluator::dead_ends_are_reliable() const {
    return true;
}

void Evaluator::report_value_for_initial_state(const EvaluationResult &result) const {
    if (enable_statistics) {
        cout << "Initial heuristic value for " << description << ": ";
        if (result.is_infinite())
            cout << "infinity";
        else
            cout << result.get_h_value();
        cout << endl;
    }
}

void Evaluator::report_progress(int value) const {
    if (enable_statistics) {
        cout << "New best heuristic value for " << description << ": "
             << value << endl;
    }
}

const string &Evaluator::get_description() const {
    return description;
}

bool Evaluator::statistics_are_enabled() const {
    return enable_statistics;
}


static PluginTypePlugin<Evaluator> _type_plugin(
    "Evaluator",
    // TODO: Replace empty string by synopsis for the wiki page.
    "");
