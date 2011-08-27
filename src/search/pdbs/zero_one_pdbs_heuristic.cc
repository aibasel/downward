#include "zero_one_pdbs_heuristic.h"

#include "pdb_heuristic.h"
#include "util.h"

#include "../globals.h"
#include "../operator.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../state.h"
#include "../utilities.h"

#include <vector>

using namespace std;

ZeroOnePDBsHeuristic::ZeroOnePDBsHeuristic(
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
    approx_mean_finite_h = 0;
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

        approx_mean_finite_h += pdb_heuristic->compute_mean_finite_h();
    }
    //cout << "All or nothing PDB collection construction time: " <<
    //timer << endl;
}

ZeroOnePDBsHeuristic::~ZeroOnePDBsHeuristic() {
    for (size_t i = 0; i < pattern_databases.size(); ++i) {
        delete pattern_databases[i];
    }
}

void ZeroOnePDBsHeuristic::initialize() {
}

int ZeroOnePDBsHeuristic::compute_heuristic(const State &state) {
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

void ZeroOnePDBsHeuristic::dump() const {
    for (size_t i = 0; i < pattern_databases.size(); ++i) {
        cout << pattern_databases[i]->get_pattern() << endl;
    }
}

static ScalarEvaluator *_parse(OptionParser &parser) {
    Heuristic::add_options_to_parser(parser);
    Options opts;
    parse_patterns(parser, opts);

    if (parser.dry_run())
        return 0;

    return new ZeroOnePDBsHeuristic(opts);
}

static Plugin<ScalarEvaluator> _plugin("zopdbs", _parse);
