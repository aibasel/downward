#include "zero_one_pdbs_heuristic.h"

#include "pdb_heuristic.h"
#include "util.h"

#include "../global_operator.h"
#include "../global_state.h"
#include "../globals.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../utilities.h"

#include <vector>

using namespace std;

ZeroOnePDBsHeuristic::ZeroOnePDBsHeuristic(
    const Options &opts,
    const vector<int> &op_costs)
    : Heuristic(opts) {
    vector<int> operator_costs;
    const OperatorsProxy &operators = task_proxy.get_operators();
    if (op_costs.empty()) { // if no operator costs are specified, use default operator costs
        operator_costs.reserve(operators.size());
        for (const OperatorProxy &op : operators)
            operator_costs.push_back(op.get_cost());
    } else {
        assert(op_costs.size() == operators.size());
        operator_costs = op_costs;
    }
    const vector<vector<int> > &pattern_collection(opts.get_list<vector<int> >("patterns"));
    //Timer timer;
    approx_mean_finite_h = 0;
    pattern_databases.reserve(pattern_collection.size());
    for (const vector<int> &pattern : pattern_collection) {
        Options opts;
        opts.set<shared_ptr<AbstractTask> >("transform", task);
        opts.set<int>("cost_type", cost_type);
        opts.set<vector<int> >("pattern", pattern);
        PDBHeuristic *pdb_heuristic = new PDBHeuristic(opts, false, operator_costs);
        pattern_databases.push_back(pdb_heuristic);

        // Set cost of relevant operators to 0 for further iterations (action cost partitioning).
        for (const OperatorProxy &op : operators) {
            if (pdb_heuristic->is_operator_relevant(op))
                operator_costs[op.get_id()] = 0;
        }

        approx_mean_finite_h += pdb_heuristic->compute_mean_finite_h();
    }
    //cout << "All or nothing PDB collection construction time: " <<
    //timer << endl;
}

ZeroOnePDBsHeuristic::~ZeroOnePDBsHeuristic() {
    for (PDBHeuristic *pdb_heuristic : pattern_databases) {
        delete pdb_heuristic;
    }
}

void ZeroOnePDBsHeuristic::initialize() {
}

int ZeroOnePDBsHeuristic::compute_heuristic(const GlobalState &global_state) {
    // since we use action cost partitioning, we can simply add up all h-values
    // from the patterns in the pattern collection
    int h_val = 0;
    for (PDBHeuristic *pdb_heuristic : pattern_databases) {
        pdb_heuristic->evaluate(global_state);
        if (pdb_heuristic->is_dead_end())
            return -1;
        h_val += pdb_heuristic->get_heuristic();
    }
    return h_val;
}

void ZeroOnePDBsHeuristic::dump() const {
    for (PDBHeuristic *pdb_heuristic : pattern_databases) {
        cout << pdb_heuristic->get_pattern() << endl;
    }
}

static Heuristic *_parse(OptionParser &parser) {
    parser.document_synopsis(
        "Zero-One PDB",
        "The zero/one pattern database heuristic is simply the sum of the "
        "heuristic values of all patterns in the pattern collection. In contrast "
        "to the canonical pattern database heuristic, there is no need to check "
        "for additive subsets, because the additivity of the patterns is "
        "guaranteed by action cost partitioning. This heuristic uses the most "
        "simple form of action cost partitioning, i.e. if an operator affects "
        "more than one pattern in the collection, its costs are entirely taken "
        "into account for one pattern (the first one which it affects) and set "
        "to zero for all other affected patterns.");
    parser.document_language_support("action costs", "supported");
    parser.document_language_support("conditional effects", "not supported");
    parser.document_language_support("axioms", "not supported");
    parser.document_property("admissible", "yes");
    parser.document_property("consistent", "yes");
    parser.document_property("safe", "yes");
    parser.document_property("preferred operators", "no");

    Heuristic::add_options_to_parser(parser);
    Options opts;
    parse_patterns(parser, opts);

    if (parser.dry_run())
        return 0;

    return new ZeroOnePDBsHeuristic(opts);
}

static Plugin<Heuristic> _plugin("zopdbs", _parse);
