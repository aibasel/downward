#include "pattern_collection_information.h"

#include "pattern_database.h"
#include "max_additive_pdb_sets.h"
#include "validation.h"

#include <algorithm>
#include <cassert>
#include <unordered_set>
#include <utility>

using namespace std;

namespace pdbs {
PatternCollectionInformation::PatternCollectionInformation(
    const TaskProxy &task_proxy,
    const shared_ptr<PatternCollection> &patterns)
    : task_proxy(task_proxy),
      patterns(patterns),
      pdbs(nullptr),
      max_additive_subsets(nullptr) {
    assert(patterns);
    validate_and_normalize_patterns(task_proxy, *patterns);
}

bool PatternCollectionInformation::information_is_valid() const {
    if (!patterns) {
        return false;
    }
    int num_patterns = patterns->size();
    if (pdbs) {
        if (patterns->size() != pdbs->size()) {
            return false;
        }
        for (int i = 0; i < num_patterns; ++i) {
            if ((*patterns)[i] != (*pdbs)[i]->get_pattern()) {
                return false;
            }
        }
    }
    if (max_additive_subsets) {
        unordered_set<PatternDatabase *> pdbs_in_union;
        for (const PDBCollection &additive_subset : *max_additive_subsets) {
            for (const shared_ptr<PatternDatabase> &pdb : additive_subset) {
                pdbs_in_union.insert(pdb.get());
            }
        }
        unordered_set<Pattern> patterns_in_union;
        for (PatternDatabase *pdb : pdbs_in_union) {
            patterns_in_union.insert(pdb->get_pattern());
        }
        unordered_set<Pattern> patterns_in_list(patterns->begin(),
                                                patterns->end());
        if (patterns_in_list != patterns_in_union) {
            return false;
        }
        if (pdbs) {
            unordered_set<PatternDatabase *> pdbs_in_list;
            for (const shared_ptr<PatternDatabase> &pdb : *pdbs) {
                pdbs_in_list.insert(pdb.get());
            }
            if (pdbs_in_list != pdbs_in_union) {
                return false;
            }
        }
    }
    return true;
}

void PatternCollectionInformation::create_pdbs_if_missing() {
    assert(patterns);
    if (!pdbs) {
        pdbs = make_shared<PDBCollection>();
        for (const Pattern &pattern : *patterns) {
            shared_ptr<PatternDatabase> pdb =
                make_shared<PatternDatabase>(task_proxy, pattern);
            pdbs->push_back(pdb);
        }
    }
}

void PatternCollectionInformation::create_max_additive_subsets_if_missing() {
    if (!max_additive_subsets) {
        create_pdbs_if_missing();
        assert(pdbs);
        VariableAdditivity are_additive = compute_additive_vars(task_proxy);
        max_additive_subsets = compute_max_additive_subsets(*pdbs, are_additive);
    }
}

void PatternCollectionInformation::set_pdbs(const shared_ptr<PDBCollection> &pdbs_) {
    pdbs = pdbs_;
    assert(information_is_valid());
}

void PatternCollectionInformation::set_max_additive_subsets(
    const shared_ptr<MaxAdditivePDBSubsets> &max_additive_subsets_) {
    max_additive_subsets = max_additive_subsets_;
    assert(information_is_valid());
}

shared_ptr<PatternCollection> PatternCollectionInformation::get_patterns() {
    assert(patterns);
    return patterns;
}

shared_ptr<PDBCollection> PatternCollectionInformation::get_pdbs() {
    create_pdbs_if_missing();
    return pdbs;
}

shared_ptr<MaxAdditivePDBSubsets> PatternCollectionInformation::get_max_additive_subsets() {
    create_max_additive_subsets_if_missing();
    return max_additive_subsets;
}
}
