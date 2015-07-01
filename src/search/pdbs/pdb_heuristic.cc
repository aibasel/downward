#include "pdb_heuristic.h"

#include "util.h"

#include "../plugin.h"
#include "../task_tools.h"
#include "../timer.h"
#include "../utilities.h"

using namespace std;

PDBHeuristic::PDBHeuristic(const Options &opts,
                           const vector<int> &operator_costs)
    : Heuristic(opts),
      pdb(task, opts.get_list<int>("pattern"), true, operator_costs) {
}

PDBHeuristic::~PDBHeuristic() {
}

void PDBHeuristic::initialize() {
}

int PDBHeuristic::compute_heuristic(const GlobalState &global_state) {
    State state = convert_global_state(global_state);
    int h = pdb.get_value(state);
    if (h == numeric_limits<int>::max())
        return DEAD_END;
    return h;
}

static Heuristic *_parse(OptionParser &parser) {
    parser.document_synopsis("Pattern database heuristic", "TODO");
    parser.document_language_support("action costs", "supported");
    parser.document_language_support("conditional effects", "not supported");
    parser.document_language_support("axioms", "not supported");
    parser.document_property("admissible", "yes");
    parser.document_property("consistent", "yes");
    parser.document_property("safe", "yes");
    parser.document_property("preferred operators", "no");

    Heuristic::add_options_to_parser(parser);
    Options opts;
    parse_pattern(parser, opts);

    if (parser.dry_run())
        return 0;

    return new PDBHeuristic(opts);
}

static Plugin<Heuristic> _plugin("pdb", _parse);
