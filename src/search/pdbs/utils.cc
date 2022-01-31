#include "utils.h"

#include "pattern_collection_information.h"
#include "pattern_database.h"
#include "pattern_information.h"

#include "../task_proxy.h"

#include "../task_utils/causal_graph.h"
#include "../task_utils/task_properties.h"

#include "../utils/logging.h"
#include "../utils/markup.h"
#include "../utils/math.h"
#include "../utils/rng.h"

#include <limits>

using namespace std;

namespace pdbs {
int compute_pdb_size(const TaskProxy &task_proxy, const Pattern &pattern) {
    int size = 1;
    for (int var : pattern) {
        int domain_size = task_proxy.get_variables()[var].get_domain_size();
        if (utils::is_product_within_limit(size, domain_size,
                                           numeric_limits<int>::max())) {
            size *= domain_size;
        } else {
            cerr << "Given pattern is too large! (Overflow occured): " << endl;
            cerr << pattern << endl;
            utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
        }
    }
    return size;
}

int compute_total_pdb_size(
    const TaskProxy &task_proxy, const PatternCollection &pattern_collection) {
    int size = 0;
    for (const Pattern &pattern : pattern_collection) {
        size += compute_pdb_size(task_proxy, pattern);
    }
    return size;
}

vector<FactPair> get_goals_in_random_order(
    const TaskProxy &task_proxy, utils::RandomNumberGenerator &rng) {
    vector<FactPair> goals = task_properties::get_fact_pairs(task_proxy.get_goals());
    rng.shuffle(goals);
    return goals;
}

vector<int> get_non_goal_variables(const TaskProxy &task_proxy) {
    size_t num_vars = task_proxy.get_variables().size();
    GoalsProxy goals = task_proxy.get_goals();
    vector<bool> is_goal(num_vars, false);
    for (FactProxy goal : goals) {
        is_goal[goal.get_variable().get_id()] = true;
    }

    vector<int> non_goal_variables;
    non_goal_variables.reserve(num_vars - goals.size());
    for (int var_id = 0; var_id < static_cast<int>(num_vars); ++var_id) {
        if (!is_goal[var_id]) {
            non_goal_variables.push_back(var_id);
        }
    }
    return non_goal_variables;
}

vector<vector<int>> compute_cg_neighbors(
    const shared_ptr<AbstractTask> &task,
    bool bidirectional) {
    TaskProxy task_proxy(*task);
    int num_vars = task_proxy.get_variables().size();
    const causal_graph::CausalGraph &cg = causal_graph::get_causal_graph(task.get());
    vector<vector<int>> cg_neighbors(num_vars);
    for (int var_id = 0; var_id < num_vars; ++var_id) {
        cg_neighbors[var_id] = cg.get_predecessors(var_id);
        if (bidirectional) {
            const vector<int> &successors = cg.get_successors(var_id);
            cg_neighbors[var_id].insert(cg_neighbors[var_id].end(), successors.begin(), successors.end());
        }
        utils::sort_unique(cg_neighbors[var_id]);
    }
    return cg_neighbors;
}

PatternCollectionInformation get_pattern_collection_info(
    const TaskProxy &task_proxy, const shared_ptr<PDBCollection> &pdbs) {
    shared_ptr<PatternCollection> patterns = make_shared<PatternCollection>();
    patterns->reserve(pdbs->size());
    for (const shared_ptr<PatternDatabase> &pdb : *pdbs) {
        patterns->push_back(pdb->get_pattern());
    }
    PatternCollectionInformation result(task_proxy, patterns);
    result.set_pdbs(pdbs);
    return result;
}

void dump_pattern_generation_statistics(
    const string &identifier,
    utils::Duration runtime,
    const PatternInformation &pattern_info) {
    const Pattern &pattern = pattern_info.get_pattern();
    utils::g_log << identifier << " pattern: " << pattern << endl;
    utils::g_log << identifier << " number of variables: " << pattern.size() << endl;
    utils::g_log << identifier << " PDB size: "
                 << compute_pdb_size(pattern_info.get_task_proxy(), pattern) << endl;
    utils::g_log << identifier << " computation time: " << runtime << endl;
}

void dump_pattern_collection_generation_statistics(
    const string &identifier,
    utils::Duration runtime,
    const PatternCollectionInformation &pci) {
    const PatternCollection &pattern_collection = *pci.get_patterns();
    utils::g_log << identifier << " number of patterns: " << pattern_collection.size()
                 << endl;
    utils::g_log << identifier << " total PDB size: ";
    utils::g_log << compute_total_pdb_size(
        pci.get_task_proxy(), pattern_collection) << endl;
    utils::g_log << identifier << " computation time: " << runtime << endl;
}

string get_rovner_et_al_reference() {
    return utils::format_conference_reference(
        {"Alexander Rovner", "Silvan Sievers", "Malte Helmert"},
        "Counterexample-Guided Abstraction Refinement for Pattern Selection "
        "in Optimal Classical Planning",
        "https://ai.dmi.unibas.ch/papers/rovner-et-al-icaps2019.pdf",
        "Proceedings of the 29th International Conference on Automated "
        "Planning and Scheduling (ICAPS 2019)",
        "362-367",
        "AAAI Press",
        "2019");
}
}
