#include "pdb_heuristic.h"

#include "pattern_generator.h"
#include "util.h"

#include "../plugin.h"
#include "../task_tools.h"
#include "../timer.h"
#include "../utilities.h"

using namespace std;

PatternDatabase get_pdb_from_options(const shared_ptr<AbstractTask> task,
                                     const Options &opts,
                                     const vector<int> &operator_costs) {
    shared_ptr<PatternGenerator> pattern_generator =
        opts.get<shared_ptr<PatternGenerator>>("pattern");
    Pattern pattern = pattern_generator->generate(task);
    TaskProxy task_proxy(*task);
    return PatternDatabase(task_proxy, pattern, true, operator_costs);
}

PDBHeuristic::PDBHeuristic(const Options &opts,
                           const vector<int> &operator_costs)
    : Heuristic(opts),
      pdb(get_pdb_from_options(task, opts, operator_costs)) {
}

PDBHeuristic::~PDBHeuristic() {
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

    parser.add_option<shared_ptr<PatternGenerator>>(
        "pattern",
        "pattern generation method",
        "greedy()");
    Heuristic::add_options_to_parser(parser);

    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;

    return new PDBHeuristic(opts);
}

static Plugin<Heuristic> _plugin("pdb", _parse);
