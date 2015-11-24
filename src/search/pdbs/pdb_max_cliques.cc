#include "pdb_max_cliques.h"

#include "max_cliques.h"
#include "pattern_database.h"

#include "../task_proxy.h"

#include <vector>

using namespace std;

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

VariableAdditivity compute_additive_vars(TaskProxy task_proxy) {
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

shared_ptr<PDBCliques> compute_max_pdb_cliques(
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

    vector<vector<int>> cgraph_max_cliques;
    compute_max_cliques(cgraph, cgraph_max_cliques);

    shared_ptr<PDBCliques> max_cliques = make_shared<PDBCliques>();
    max_cliques->reserve(cgraph_max_cliques.size());
    for (const vector<int> &cgraph_max_clique : cgraph_max_cliques) {
        PDBCollection clique;
        clique.reserve(cgraph_max_clique.size());
        for (int pdb_id : cgraph_max_clique) {
            clique.push_back(pdbs[pdb_id]);
        }
        max_cliques->push_back(clique);
    }
    return max_cliques;
}

PDBCliques get_max_additive_subsets(
    const PDBCliques &max_cliques, const Pattern &new_pattern,
    const VariableAdditivity &are_additive) {
    PDBCliques max_additive_subsets;
    for (const auto &clique : max_cliques) {
        // Take all patterns which are additive to new_pattern.
        PDBCollection subset;
        subset.reserve(clique.size());
        for (const auto &pdb : clique) {
            if (are_patterns_additive(
                    new_pattern, pdb->get_pattern(), are_additive)) {
                subset.push_back(pdb);
            }
        }
        if (!subset.empty()) {
            max_additive_subsets.push_back(subset);
        }
    }
    if (max_additive_subsets.empty()) {
        // If nothing was additive with the new variable, then
        // the only additive subset is the empty set.
        max_additive_subsets.emplace_back();
    }
    return max_additive_subsets;
}
