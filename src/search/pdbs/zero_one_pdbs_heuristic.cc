#include "zero_one_pdbs_heuristic.h"

#include "pdb_heuristic.h"
#include "util.h"

#include "../evaluation_context.h"
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
        opts.set<shared_ptr<AbstractTask> >("transform", task);
        opts.set<int>("cost_type", cost_type);
        opts.set<vector<int> >("pattern", pattern_collection[i]);
        PDBHeuristic *pdb_heuristic = new PDBHeuristic(opts, false, operator_costs);
        pattern_databases.push_back(pdb_heuristic);

        // Set cost of relevant operators to 0 for further iterations (action cost partitioning).
        for (size_t j = 0; j < g_operators.size(); ++j) {
            if (pdb_heuristic->is_operator_relevant(g_operators[j]))
                operator_costs[j] = 0;
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

int ZeroOnePDBsHeuristic::compute_heuristic(const GlobalState &state) {
    /*
      Because we use cost partitioning, we can simply add up all
      heuristic values of all patterns in the pattern collection.

      The use of evaluation contexts maybe makes this a bit less
      efficient than it ought to be. See discussion in
      CanonicalPDBsHeuristic::compute_heuristic.
    */

    EvaluationContext eval_context(state);
    int h_val = 0;
    for (PDBHeuristic *pdb : pattern_databases) {
        if (eval_context.is_heuristic_infinite(pdb))
            return -1;
        h_val += eval_context.get_heuristic_value(pdb);
    }
    return h_val;
}

void ZeroOnePDBsHeuristic::dump() const {
    for (size_t i = 0; i < pattern_databases.size(); ++i) {
        cout << pattern_databases[i]->get_pattern() << endl;
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
