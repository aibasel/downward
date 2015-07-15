#include "search_progress.h"

#include "evaluation_context.h"
#include "heuristic.h"

#include <iostream>
#include <string>
using namespace std;


bool SearchProgress::process_heuristic_value(const Heuristic *heuristic, int h) {
    /*
      Handle one heuristic value:
      1. insert into or update best_heuristic_values if necessary
      2. return true if this is a new best heuristic value
         (includes case where we haven't seen this heuristic before)
    */
    auto insert_result = best_heuristic_values.insert(make_pair(heuristic, h));
    auto iter = insert_result.first;
    bool was_inserted = insert_result.second;
    if (was_inserted) {
        // We haven't seen this heuristic before.
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

bool SearchProgress::check_progress(const EvaluationContext &eval_context) {
    bool progress = false;
    eval_context.get_cache().for_each_heuristic_value(
        [this, &progress](const Heuristic * heur, const EvaluationResult &result) {
            int h = result.get_h_value();
            if (process_heuristic_value(heur, h)) {
                cout << "New best heuristic value for "
                     << heur->get_description() << ": " << h << endl;
                progress = true;
            }
        }
        );
    return progress;
}
