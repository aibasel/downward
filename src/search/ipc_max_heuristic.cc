#include "ipc_max_heuristic.h"

#include "eager_evaluation_context.h"
#include "evaluation_result.h"
#include "option_parser.h"
#include "plugin.h"

#include <limits>
#include <string>
#include <vector>

using namespace std;


IPCMaxHeuristic::IPCMaxHeuristic(const Options &opts)
    : Heuristic(opts),
      heuristics(opts.get_list<Heuristic *>("heuristics")) {
}

IPCMaxHeuristic::~IPCMaxHeuristic() {
}

int IPCMaxHeuristic::compute_heuristic(const GlobalState &state) {
    int value = 0;
    for (Heuristic *heuristic : heuristics) {
        // Pass arbitrary values for g and is_preferred (ignored by heuristics).
        EagerEvaluationContext eval_context(state, -1, true, nullptr);
        EvaluationResult result = heuristic->compute_result(eval_context);
        value = max(value, result.get_h_value());
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
    parser.document_hide(); //don't show documentation for this temporary class (see issue198)
    parser.document_synopsis("IPC-Max Heuristic", "");
    parser.add_list_option<Heuristic *>("heuristics");
    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();

    opts.verify_list_non_empty<Heuristic *>("heuristics");

    if (parser.dry_run())
        return 0;
    else
        return new IPCMaxHeuristic(opts);
}

static Plugin<Heuristic> plugin("max", _parse);
