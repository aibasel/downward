#include "search_progress.h"

#include "evaluation_context.h"
#include "heuristic.h"

#include <iostream>
#include <string>
using namespace std;


bool SearchProgress::process_heuristic_value(const Heuristic *heuristic, int h) {
    /*
      Handle one heuristic value:
      1. insert into initial_heuristic_values if necessary
      2. insert into or update best_heuristic_values if necessary
      3. return true if this is a new best heuristic value
         (includes case where we haven't seen this heuristic before)
    */
    auto insert_result = best_heuristic_values.insert(make_pair(heuristic, h));
    auto iter = insert_result.first;
    bool was_inserted = insert_result.second;
    if (was_inserted) {
        // We haven't seen this heuristic before.
        initial_heuristic_values[heuristic] = h;
        return true;
    } else {
        int &best_h = iter->second;
        if (h < best_h) {
            best_h = h;
            return true;
        }
    }
    return false;
}

void SearchProgress::output_line(const string &description,
                                 const Heuristic *heuristic, int h) const {
    cout << description << " heuristic value for "
         << heuristic->get_description() << ": " << h << endl;
}

bool SearchProgress::check_progress(const EvaluationContext &eval_context) {
    bool result = false;
    for (const auto &element : eval_context.get_cache().get_eval_results()) {
        const ScalarEvaluator *evaluator = element.first;
        int h = element.second.get_h_value();
        const Heuristic *heur = dynamic_cast<const Heuristic *>(evaluator);
        if (heur) {
            if (process_heuristic_value(heur, h)) {
                output_line("New best", heur, h);
                result = true;
            }
        }
    }
    return result;
}

void SearchProgress::print_initial_h_values() const {
    for (const auto &entry : initial_heuristic_values) {
        const Heuristic *heur = entry.first;
        int h = entry.second;
        output_line("Initial", heur, h);
    }
}
