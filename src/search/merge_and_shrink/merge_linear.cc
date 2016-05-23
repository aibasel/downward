#include "merge_linear.h"

#include "factored_transition_system.h"

#include "../variable_order_finder.h"

#include <cassert>
#include <iostream>

using namespace std;

namespace merge_and_shrink {
MergeLinear::MergeLinear(FactoredTransitionSystem &fts,
                         unique_ptr<VariableOrderFinder> variable_order_finder)
    : MergeStrategy(fts),
      variable_order_finder(move(variable_order_finder)),
      need_first_index(true) {
}

pair<int, int> MergeLinear::get_next() {
    int num_transition_systems = fts.get_size();
    assert(!variable_order_finder->done());

    int first;
    if (need_first_index) {
        need_first_index = false;
        first = variable_order_finder->next();
        cout << "First variable: " << first << endl;
    } else {
        // The most recent composite transition system is appended at the end of
        // all_transition_systems in merge_and_shrink.cc
        first = num_transition_systems - 1;
    }
    int second = variable_order_finder->next();
    cout << "Next variable: " << second << endl;
    assert(fts.is_active(first));
    assert(fts.is_active(second));
    return make_pair(first, second);
}
}
