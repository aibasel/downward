#include "utils.h"

#include "pattern_collection_information.h"
#include "pattern_database.h"
#include "pattern_information.h"

#include "../utils/logging.h"
#include "../utils/math.h"
#include "../utils/rng.h"

#include "../task_proxy.h"

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
    const TaskProxy &task_proxy, const shared_ptr<utils::RandomNumberGenerator> &rng) {
    vector<FactPair> goals;
    for (FactProxy goal : task_proxy.get_goals()) {
        goals.push_back(goal.get_pair());
    }
    rng->shuffle(goals);
    return goals;
}

vector<int> get_non_goal_variables(const TaskProxy &task_proxy) {
    GoalsProxy goals = task_proxy.get_goals();
    int num_vars = task_proxy.get_variables().size();
    vector<int> non_goal_variables;
    non_goal_variables.reserve(num_vars - goals.size());
    for (int var_id = 0; var_id < num_vars; ++var_id) {
        bool is_goal_var = false;
        for (FactProxy goal : goals) {
            if (var_id == goal.get_variable().get_id()) {
                is_goal_var = true;
                break;
            }
        }
        if (!is_goal_var) {
            non_goal_variables.push_back(var_id);
        }
    }
    return non_goal_variables;
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
    const PatternCollectionInformation &pci,
    int collection_size,
    bool dump_collection) {
    const PatternCollection &pattern_collection = *pci.get_patterns();
    if (dump_collection) {
        utils::g_log << identifier << " collection: " << pattern_collection << endl;
    }
    utils::g_log << identifier << " number of patterns: " << pattern_collection.size()
                 << endl;
    utils::g_log << identifier << " total PDB size: ";
    if (collection_size != -1) {
        utils::g_log << collection_size << endl;
    } else {
        utils::g_log << compute_total_pdb_size(
            pci.get_task_proxy(), pattern_collection) << endl;
    }
    utils::g_log << identifier << " computation time: " << runtime << endl;
}
}
