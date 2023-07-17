#include "merge_selector.h"

#include "factored_transition_system.h"
#include "merge_scoring_function.h"

#include "../task_proxy.h"
#include "../plugins/plugin.h"
#include "../utils/collections.h"
#include "../utils/logging.h"

#include <cassert>
#include <iostream>

using namespace std;

namespace merge_and_shrink {
shared_ptr<MergeCandidate> MergeSelector::get_candidate(
    int index1, int index2) {
    assert(utils::in_bounds(index1, merge_candidates_by_indices));
    assert(utils::in_bounds(index2, merge_candidates_by_indices[index1]));
    if (merge_candidates_by_indices[index1][index2] == nullptr) {
        merge_candidates_by_indices[index1][index2] =
            make_shared<MergeCandidate>(num_candidates, index1, index2);
        ++num_candidates;
    }
    return merge_candidates_by_indices[index1][index2];
}

vector<shared_ptr<MergeCandidate>> MergeSelector::compute_merge_candidates(
    const FactoredTransitionSystem &fts,
    const vector<int> &indices_subset) {
    vector<shared_ptr<MergeCandidate>> merge_candidates;
    if (indices_subset.empty()) {
        for (int ts_index1 = 0; ts_index1 < fts.get_size(); ++ts_index1) {
            if (fts.is_active(ts_index1)) {
                for (int ts_index2 = ts_index1 + 1; ts_index2 < fts.get_size();
                     ++ts_index2) {
                    if (fts.is_active(ts_index2)) {
                        merge_candidates.push_back(get_candidate(ts_index1, ts_index2));
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
                merge_candidates.push_back(get_candidate(ts_index1, ts_index2));
            }
        }
    }
    return merge_candidates;
}

void MergeSelector::initialize(const TaskProxy &task_proxy) {
    int num_variables = task_proxy.get_variables().size();
    int max_factor_index = 2 * num_variables - 1;
    merge_candidates_by_indices.resize(
        max_factor_index,
        vector<shared_ptr<MergeCandidate>>(max_factor_index, nullptr));
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
