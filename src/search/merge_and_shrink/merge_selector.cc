#include "merge_selector.h"

#include "factored_transition_system.h"

#include "../options/plugin.h"

#include <cassert>
#include <iostream>

using namespace std;

namespace merge_and_shrink {
vector<pair<int, int>> MergeSelector::compute_merge_candidates(
    const FactoredTransitionSystem &fts,
    const vector<int> &indices_subset) const {
    vector<pair<int, int>> merge_candidates;
    if (indices_subset.empty()) {
        for (int ts_index1 = 0; ts_index1 < fts.get_size(); ++ts_index1) {
            if (fts.is_active(ts_index1)) {
                for (int ts_index2 = ts_index1 + 1; ts_index2 < fts.get_size();
                     ++ts_index2) {
                    if (fts.is_active(ts_index2)) {
                        merge_candidates.emplace_back(ts_index1, ts_index2);
                    }
                }
            }
        }
    } else {
        assert(indices_subset.size() > 1);
        for (size_t i = 0; i < indices_subset.size(); ++i) {
            int ts_index1 = indices_subset[i];
            assert(fts.is_active(ts_index1));
            for (size_t j = i + 1; j < indices_subset.size(); ++j) {
                int ts_index2 = indices_subset[j];
                assert(fts.is_active(ts_index2));
                merge_candidates.emplace_back(ts_index1, ts_index2);
            }
        }
    }
    return merge_candidates;
}

void MergeSelector::dump_options() const {
    cout << "Merge selector options:" << endl;
    cout << "Name: " << name() << endl;
    dump_specific_options();
}

static options::PluginTypePlugin<MergeSelector> _type_plugin(
    "MergeSelector",
    "This page describes the available merge selectors. They are used to "
    "compute the next merge purely based on the state of the given factored "
    "transition system. They are used in the merge strategy of type "
    "'stateless', but they can also easily be used in different 'combined'"
    "merged strategies.");
}
