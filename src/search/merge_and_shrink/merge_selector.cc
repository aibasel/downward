#include "merge_selector.h"

#include "factored_transition_system.h"

#include "../plugins/plugin.h"
#include "../utils/logging.h"

#include <cassert>
#include <iostream>

using namespace std;

namespace merge_and_shrink {
vector<pair<int, int>> MergeSelector::compute_merge_candidates(
    const FactoredTransitionSystem &fts) const {
    vector<int> indices;
    indices.reserve(fts.get_num_active_entries());
    for (int index: fts) {
        indices.push_back(index);
    }

    vector<pair<int, int>> merge_candidates;
    merge_candidates.reserve(indices.size() * (indices.size() - 1) / 2);
    for (size_t i = 0; i < indices.size(); ++i) {
        int ts_index1 = indices[i];
        assert(fts.is_active(ts_index1));
        for (size_t j = i + 1; j < indices.size(); ++j) {
            int ts_index2 = indices[j];
            assert(fts.is_active(ts_index2));
            merge_candidates.emplace_back(ts_index1, ts_index2);
        }
    }
    return merge_candidates;
}

void MergeSelector::dump_options(utils::LogProxy &log) const {
    if (log.is_at_least_normal()) {
        log << "Merge selector options:" << endl;
        log << "Name: " << name() << endl;
        dump_selector_specific_options(log);
    }
}

static class MergeSelectorCategoryPlugin : public plugins::TypedCategoryPlugin<MergeSelector> {
public:
    MergeSelectorCategoryPlugin() : TypedCategoryPlugin("MergeSelector") {
        document_synopsis(
            "This page describes the available merge selectors. They are used to "
            "compute the next merge purely based on the state of the given factored "
            "transition system. They are used in the merge strategy of type "
            "'stateless', but they can also easily be used in different 'combined' "
            "merged strategies.");
    }
}
_category_plugin;
}
