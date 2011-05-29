#include "zero_one_partitioning_pdb_collection_heuristic.h"
#include "globals.h"
#include "operator.h"
#include "option_parser.h"
#include "pdb_heuristic.h"
#include "plugin.h"
#include "state.h"
#include "timer.h"

#include <vector>

using namespace std;

ZeroOnePartitioningPdbCollectionHeuristic::ZeroOnePartitioningPdbCollectionHeuristic(
    const Options &opts,
    const vector<int> &op_costs)
    : Heuristic(opts) {
    vector<int> operator_costs;
    if (op_costs.empty()) { // if no operator costs are specified, use default operator costs
        operator_costs.reserve(g_operators.size());
        for (size_t i = 0; i < g_operators.size(); ++i)
            operator_costs.push_back(get_adjusted_cost(g_operators[i]));
    } else {
        assert(op_costs.size() == g_operators.size());
        operator_costs = op_costs;
    }
    const vector<vector<int> > &pattern_collection(opts.get_list<vector<int> >("patterns"));
    //Timer timer;
    fitness = 0;
    pattern_databases.reserve(pattern_collection.size());
    for (size_t i = 0; i < pattern_collection.size(); ++i) {
        Options opts;
        opts.set<int>("cost_type", cost_type);
        opts.set<vector<int> >("pattern", pattern_collection[i]);
        PDBHeuristic *pdb_heuristic = new PDBHeuristic(opts, false, operator_costs);
        pattern_databases.push_back(pdb_heuristic);

        // get used operators and set their cost for further iterations to 0 (action cost partitioning)
        const vector<bool> &used_ops = pdb_heuristic->get_relevant_operators();
        assert(used_ops.size() == operator_costs.size());
        for (size_t k = 0; k < used_ops.size(); ++k) {
            if (used_ops[k])
                operator_costs[k] = 0;
        }

        fitness += pdb_heuristic->compute_mean_h();
    }
    //cout << "All or nothing PDB collection construction time: " << 
//timer << endl;
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
        cout << "[ ";
        for (size_t j = 0; j < pattern.size(); ++j) {
            cout << pattern[j] << " ";
        }
        cout << "]" << endl;
    }
}

static ScalarEvaluator *_parse(OptionParser &parser) {
    parser.add_list_option<vector<int> >("patterns", vector<vector<int> >(), "the pattern collection");
    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();

    vector<vector<int> > pattern_collection = opts.get_list<vector<int> >("patterns");
    // TODO: Distinguish the case that no collection was specified by
    // the user from the case that an empty collection was explicitly
    // specified by the user (see issue236).
    if (parser.dry_run() && !pattern_collection.empty()) {
        // check if there are duplicates of patterns
        for (size_t i = 0; i < pattern_collection.size(); ++i) {
            sort(pattern_collection[i].begin(), pattern_collection[i].end());
            // check if each pattern is valid
            int pat_old_size = pattern_collection[i].size();
            vector<int>::const_iterator it = unique(pattern_collection[i].begin(), pattern_collection[i].end());
            pattern_collection[i].resize(it - pattern_collection[i].begin());
            if (pattern_collection[i].size() != pat_old_size)
                parser.error("there are duplicates of variables in a pattern");
            if (!pattern_collection[i].empty()) {
                if (pattern_collection[i].front() < 0)
                    parser.error("there is a variable < 0 in a pattern");
                if (pattern_collection[i].back() >= g_variable_domain.size())
                    parser.error("there is a variable > number of variables in a pattern");
            }
        }
        sort(pattern_collection.begin(), pattern_collection.end());
        int coll_old_size = pattern_collection.size();
        vector<vector<int> >::const_iterator it = unique(pattern_collection.begin(), pattern_collection.end());
        pattern_collection.resize(it - pattern_collection.begin());
        if (pattern_collection.size() != coll_old_size)
            parser.error("there are duplicates of patterns in the pattern collection");

        for (size_t i = 0; i < pattern_collection.size(); ++i) {
            cout << "[ ";
            for (size_t j = 0; j < pattern_collection[i].size(); ++j) {
                cout << pattern_collection[i][j] << " ";
            }
            cout << "]" << endl;
        }
    }

    if (parser.dry_run())
        return 0;

    // TODO: See above (issue236).
    if (pattern_collection.empty()) {
        // Simple selection strategy. Take all goal variables as patterns.
        for (size_t i = 0; i < g_goal.size(); ++i) {
            pattern_collection.push_back(vector<int>(1, g_goal[i].first));
        }
        opts.set("patterns", pattern_collection);
    }

    return new ZeroOnePartitioningPdbCollectionHeuristic(opts);
}

static Plugin<ScalarEvaluator> _plugin("zoppch", _parse);
