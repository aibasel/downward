#include "pattern_collection_information.h"

#include "pattern_database.h"
#include "pattern_cliques.h"
#include "validation.h"

#include "../utils/logging.h"
#include "../utils/timer.h"

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
      pattern_cliques(nullptr) {
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
    if (pattern_cliques) {
        for (const PatternClique &clique : *pattern_cliques) {
            for (PatternID pattern_id : clique) {
                if (!utils::in_bounds(pattern_id, *patterns)) {
                    return false;
                }
            }
        }
    }
    return true;
}

void PatternCollectionInformation::create_pdbs_if_missing() {
    assert(patterns);
    if (!pdbs) {
        utils::Timer timer;
        utils::g_log << "Computing PDBs for pattern collection..." << endl;
        pdbs = make_shared<PDBCollection>();
        for (const Pattern &pattern : *patterns) {
            shared_ptr<PatternDatabase> pdb =
                make_shared<PatternDatabase>(task_proxy, pattern);
            pdbs->push_back(pdb);
        }
        utils::g_log << "Done computing PDBs for pattern collection: " << timer << endl;
    }
}

void PatternCollectionInformation::create_pattern_cliques_if_missing() {
    if (!pattern_cliques) {
        utils::Timer timer;
        utils::g_log << "Computing pattern cliques for pattern collection..." << endl;
        VariableAdditivity are_additive = compute_additive_vars(task_proxy);
        pattern_cliques = compute_pattern_cliques(*patterns, are_additive);
        utils::g_log << "Done computing pattern cliques for pattern collection: "
                     << timer << endl;
    }
}

void PatternCollectionInformation::set_pdbs(const shared_ptr<PDBCollection> &pdbs_) {
    pdbs = pdbs_;
    assert(information_is_valid());
}

void PatternCollectionInformation::set_pattern_cliques(
    const shared_ptr<vector<PatternClique>> &pattern_cliques_) {
    pattern_cliques = pattern_cliques_;
    assert(information_is_valid());
}

shared_ptr<PatternCollection> PatternCollectionInformation::get_patterns() const {
    assert(patterns);
    return patterns;
}

shared_ptr<PDBCollection> PatternCollectionInformation::get_pdbs() {
    create_pdbs_if_missing();
    return pdbs;
}

shared_ptr<vector<PatternClique>> PatternCollectionInformation::get_pattern_cliques() {
    create_pattern_cliques_if_missing();
    return pattern_cliques;
}
}
