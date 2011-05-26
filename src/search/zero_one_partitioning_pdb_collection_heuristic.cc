#include "zero_one_partitioning_pdb_collection_heuristic.h"
#include "option_parser.h"
#include "pdb_heuristic.h"
#include "state.h"
#include "timer.h"

#include <vector>

using namespace std;

ZeroOnePartitioningPdbCollectionHeuristic::ZeroOnePartitioningPdbCollectionHeuristic(const Options &opts)
    : Heuristic(opts) {
    const vector<vector<int> > &pattern_collection(opts.get_list<vector<int> >("patterns"));
    vector<int> op_costs = opts.get_list<int>("op_costs");
    Timer timer;
    fitness = 0;
    pattern_databases.reserve(pattern_collection.size());
    for (size_t i = 0; i < pattern_collection.size(); ++i) {
        Options opts;
        opts.set<int>("cost_type", cost_type);
        opts.set<vector<int> >("pattern", pattern_collection[i]);
        PDBHeuristic *pdb_heuristic = new PDBHeuristic(opts, false, op_costs);
        pattern_databases.push_back(pdb_heuristic);

        // get used operators and set their cost for further iterations to 0 (action cost partitioning)
        const vector<bool> &used_ops = pdb_heuristic->get_relevant_operators();
        assert(used_ops.size() == op_costs.size());
        for (size_t k = 0; k < used_ops.size(); ++k) {
            if (used_ops[k])
                op_costs[k] = 0;
        }

        fitness += pdb_heuristic->compute_mean_h();
    }
    cout << "All or nothing PDB collection construction time: " << timer << endl;
}

ZeroOnePartitioningPdbCollectionHeuristic::~ZeroOnePartitioningPdbCollectionHeuristic() {
    for (size_t i = 0; i < pattern_databases.size(); ++i) {
        delete pattern_databases[i];
    }
}

void ZeroOnePartitioningPdbCollectionHeuristic::initialize() {
}

int ZeroOnePartitioningPdbCollectionHeuristic::compute_heuristic(const State &state) {
    // since we use action cost partitioning, we can simply add up all h-values
    // from the patterns in the pattern collection
    int h_val = 0;
    for (size_t i = 0; i < pattern_databases.size(); ++i) {
        pattern_databases[i]->evaluate(state);
        if (pattern_databases[i]->is_dead_end())
            return -1;
        h_val += pattern_databases[i]->get_heuristic();
    }
    return h_val;
}

void ZeroOnePartitioningPdbCollectionHeuristic::dump() const {
    for (size_t i = 0; i < pattern_databases.size(); ++i) {
        const vector<int> &pattern = pattern_databases[i]->get_pattern();
        cout << "Pattern " << i << ": [ ";
        for (size_t j = 0; j < pattern.size(); ++j) {
            cout << pattern[j] << " ";
        }
        cout << "]" << endl;
    }
}
