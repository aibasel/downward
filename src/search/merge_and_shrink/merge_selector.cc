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
    vector<pair<int, int>> merge_candidates;
    merge_candidates.reserve(
        (fts.get_num_active_entries() * (fts.get_num_active_entries() - 1)) /
        2);
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
    return merge_candidates;
}

pair<int, int> MergeSelector::select_merge(
    const FactoredTransitionSystem &fts) const {
    vector<pair<int, int>> merge_candidates = compute_merge_candidates(fts);
    return select_merge_from_candidates(fts, move(merge_candidates));
}

void MergeSelector::dump_options(utils::LogProxy &log) const {
    if (log.is_at_least_normal()) {
        log << "Merge selector options:" << endl;
        log << "Name: " << name() << endl;
        dump_selector_specific_options(log);
    }
}

static class MergeSelectorCategoryPlugin
    : public plugins::TypedCategoryPlugin<MergeSelector> {
public:
    MergeSelectorCategoryPlugin() : TypedCategoryPlugin("MergeSelector") {
        document_synopsis(
            "This page describes the available merge selectors. They are used to "
            "compute the next merge purely based on the state of the given factored "
            "transition system. They are used in the merge strategy of type "
            "'stateless', but they can also easily be used in different 'combined' "
            "merged strategies.");
    }
} _category_plugin;
}
