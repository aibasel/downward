#include "utils.h"

#include "pattern_collection_information.h"
#include "pattern_database.h"
#include "pattern_information.h"

#include "../utils/logging.h"

#include "../task_proxy.h"

using namespace std;

namespace pdbs {
int compute_pdb_size(const TaskProxy &task_proxy, const Pattern &pattern) {
    int size = 1;
    for (int var : pattern) {
        size *= task_proxy.get_variables()[var].get_domain_size();
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

void dump_pattern_generation_statistics(
    const string &identifier,
    utils::Duration runtime,
    const PatternInformation &pattern_info) {
    const Pattern &pattern = pattern_info.get_pattern();
    cout << identifier << " pattern: " << pattern << endl;
    cout << identifier << " number of variables: " << pattern.size() << endl;
    cout << identifier << " PDB size: "
         << compute_pdb_size(pattern_info.get_task_proxy(), pattern) << endl;
    cout << identifier << " computation time: " << runtime << endl;
}

void dump_pattern_collection_generation_statistics(
    const string &identifier,
    utils::Duration runtime,
    const PatternCollectionInformation &pci,
    bool dump_collection) {
    const PatternCollection &pattern_collection = *pci.get_patterns();
    if (dump_collection) {
        cout << identifier << " collection: " << pattern_collection << endl;
    }
    cout << identifier << " number of patterns: " << pattern_collection.size()
         << endl;
    cout << identifier << " total PDB size: "
         << compute_total_pdb_size(pci.get_task_proxy(), pattern_collection)
         << endl;
    cout << identifier << " computation time: " << runtime << endl;
}
}
