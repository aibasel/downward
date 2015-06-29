#include "ipc_max_heuristic.h"

#include "evaluation_context.h"
#include "option_parser.h"
#include "plugin.h"

using namespace std;


IPCMaxHeuristic::IPCMaxHeuristic(const Options &opts)
    : Heuristic(opts),
      heuristics(opts.get_list<Heuristic *>("heuristics")) {
}

int IPCMaxHeuristic::compute_heuristic(const GlobalState &state) {
    int value = 0;
    EvaluationContext eval_context(state);
    for (Heuristic *heuristic : heuristics) {
        value = max(value, eval_context.get_heuristic_value(heuristic));
    }
    return value;
}

bool IPCMaxHeuristic::reach_state(const GlobalState &parent_state,
                                  const GlobalOperator &op,
                                  const GlobalState &state) {
    bool result = false;
    for (Heuristic *heuristic : heuristics) {
        if (heuristic->reach_state(parent_state, op, state)) {
            result = true;
            // Don't break: we must call reached_state everywhere.
        }
    }
    return result;
}

static Heuristic *_parse(OptionParser &parser) {
    // Don't show documentation for this temporary class (see issue198).
    parser.document_hide();
    parser.document_synopsis("IPC-Max Heuristic", "");
    parser.add_list_option<Heuristic *>("heuristics");
    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();

    opts.verify_list_non_empty<Heuristic *>("heuristics");

    if (parser.dry_run())
        return nullptr;
    else
        return new IPCMaxHeuristic(opts);
}

static Plugin<Heuristic> plugin("max", _parse);
