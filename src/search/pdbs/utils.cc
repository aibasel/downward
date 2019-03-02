#include "utils.h"

#include "pattern_database.h"

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
    const TaskProxy &task_proxy,
    string identifier,
    utils::Duration runtime,
    const Pattern &pattern,
    const shared_ptr<PatternDatabase> &pdb) {
    int pdb_size;
    if (pdb) {
        pdb_size = pdb->get_size();
    } else {
        pdb_size = compute_pdb_size(task_proxy, pattern);
    }
    cout << identifier << " pattern: " << pattern << endl;
    cout << identifier << " PDB size: " << pdb_size << endl;
    cout << identifier << " computation time: " << runtime << endl;
}

void dump_pattern_collection_generation_statistics(
    const TaskProxy &task_proxy,
    string identifier,
    utils::Duration runtime,
    const std::shared_ptr<PatternCollection> &pattern_collection,
    const std::shared_ptr<PDBCollection> &pdbs) {
    assert(pattern_collection || pdbs);

    int num_patterns;
    int total_pdb_size = 0;
    if (pdbs) {
        for (const auto &pdb : *pdbs) {
            total_pdb_size += pdb->get_size();
        }
        num_patterns = pdbs->size();
    } else {
        total_pdb_size = compute_total_pdb_size(task_proxy, *pattern_collection);
        num_patterns = pattern_collection->size();
    }

    cout << identifier << " collection: ";
    if (pattern_collection) {
        cout << *pattern_collection << endl;
    } else {
        // TODO: have a more generic way of printing sequences whose elements
        // cannot directly be printed on ostream?
        cout << "[";
        string sep = "";
        for (const auto &pdb : *pdbs) {
            cout << sep << pdb->get_pattern();
            sep = ", ";
        }
        cout << "]" << endl;
    }
    cout << identifier << " number of patterns: " << num_patterns << endl;
    cout << identifier << " total PDB size: " << total_pdb_size << endl;
    cout << identifier << " computation time: " << runtime << endl;
}
}
