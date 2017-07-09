#include "max_additive_pdb_sets.h"

#include "pattern_database.h"

#include "../task_proxy.h"

#include "../algorithms/max_cliques.h"

using namespace std;

namespace pdbs {
bool are_patterns_additive(const Pattern &pattern1,
                           const Pattern &pattern2,
                           const VariableAdditivity &are_additive) {
    for (int v1 : pattern1) {
        for (int v2 : pattern2) {
            if (!are_additive[v1][v2]) {
                return false;
            }
        }
    }
    return true;
}

VariableAdditivity compute_additive_vars(const TaskProxy &task_proxy) {
    VariableAdditivity are_additive;
    int num_vars = task_proxy.get_variables().size();
    are_additive.resize(num_vars, vector<bool>(num_vars, true));
    for (OperatorProxy op : task_proxy.get_operators()) {
        for (EffectProxy e1 : op.get_effects()) {
            for (EffectProxy e2 : op.get_effects()) {
                int e1_var_id = e1.get_fact().get_variable().get_id();
                int e2_var_id = e2.get_fact().get_variable().get_id();
                are_additive[e1_var_id][e2_var_id] = false;
            }
        }
    }
    return are_additive;
}

shared_ptr<MaxAdditivePDBSubsets> compute_max_additive_subsets(
    const PDBCollection &pdbs, const VariableAdditivity &are_additive) {
    // Initialize compatibility graph.
    vector<vector<int>> cgraph;
    cgraph.resize(pdbs.size());

    for (size_t i = 0; i < pdbs.size(); ++i) {
        for (size_t j = i + 1; j < pdbs.size(); ++j) {
            if (are_patterns_additive(pdbs[i]->get_pattern(),
                                      pdbs[j]->get_pattern(),
                                      are_additive)) {
                /* If the two patterns are additive, there is an edge in the
                   compatibility graph. */
                cgraph[i].push_back(j);
                cgraph[j].push_back(i);
            }
        }
    }

    vector<vector<int>> max_cliques;
    max_cliques::compute_max_cliques(cgraph, max_cliques);

    shared_ptr<MaxAdditivePDBSubsets> max_additive_sets =
        make_shared<MaxAdditivePDBSubsets>();
    max_additive_sets->reserve(max_cliques.size());
    for (const vector<int> &max_clique : max_cliques) {
        PDBCollection max_additive_subset;
        max_additive_subset.reserve(max_clique.size());
        for (int pdb_id : max_clique) {
            max_additive_subset.push_back(pdbs[pdb_id]);
        }
        max_additive_sets->push_back(max_additive_subset);
    }
    return max_additive_sets;
}

MaxAdditivePDBSubsets compute_max_additive_subsets_with_pattern(
    const MaxAdditivePDBSubsets &known_additive_subsets,
    const Pattern &new_pattern,
    const VariableAdditivity &are_additive) {
    MaxAdditivePDBSubsets subsets_additive_with_pattern;
    for (const auto &known_subset : known_additive_subsets) {
        // Take all patterns which are additive to new_pattern.
        PDBCollection new_subset;
        new_subset.reserve(known_subset.size());
        for (const shared_ptr<PatternDatabase> &pdb : known_subset) {
            if (are_patterns_additive(
                    new_pattern, pdb->get_pattern(), are_additive)) {
                new_subset.push_back(pdb);
            }
        }
        if (!new_subset.empty()) {
            subsets_additive_with_pattern.push_back(new_subset);
        }
    }
    if (subsets_additive_with_pattern.empty()) {
        // If nothing was additive with the new variable, then
        // the only additive subset is the empty set.
        subsets_additive_with_pattern.emplace_back();
    }
    return subsets_additive_with_pattern;
}
}
