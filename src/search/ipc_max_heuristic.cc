#include "ipc_max_heuristic.h"

#include "option_parser.h"
#include "plugin.h"

#include <limits>
#include <string>
#include <vector>

using namespace std;


IPCMaxHeuristic::IPCMaxHeuristic(const HeuristicOptions &options,
                                 const vector<Heuristic *> &evals)
    : Heuristic(options), evaluators(evals) {
}

IPCMaxHeuristic::~IPCMaxHeuristic() {
}

int IPCMaxHeuristic::compute_heuristic(const State &state) {
    dead_end = false;
    dead_end_reliable = false;
    value = 0;
    for (unsigned int i = 0; i < evaluators.size(); i++) {
        evaluators[i]->evaluate(state);

        if (evaluators[i]->is_dead_end()) {
            value = numeric_limits<int>::max();
            dead_end = true;
            if (evaluators[i]->dead_end_is_reliable()) {
                dead_end_reliable = true;
                value = -1;
                break;
            }
        } else {
            value = max(value, evaluators[i]->get_value());
        }
    }
    return value;
}

bool IPCMaxHeuristic::reach_state(const State &parent_state, const Operator &op,
                                  const State &state) {
    bool result = false;
    for (int i = 0; i < evaluators.size(); i++) {
        if (evaluators[i]->reach_state(parent_state, op, state)) {
            result = true;
            // Don't break: we must call reached_state everywhere.
        }
    }
    return result;
}

static ScalarEvaluator *create(const vector<string> &config,
                               int start, int &end, bool dry_run) {
    if (config[start + 1] != "(") {
        throw ParseError(start + 1);
    }

    vector<Heuristic *> heuristics_;
    OptionParser::instance()->parse_heuristic_list(config, start + 2,
                                                   end, false, heuristics_,
                                                   dry_run);
    if (heuristics_.empty()) {
        throw ParseError(end);
    }
    end++;

    HeuristicOptions common_options;


    if (config[end] != ")") {
        end++;
        NamedOptionParser option_parser;

        common_options.add_option_to_parser(option_parser);

        option_parser.parse_options(config, end, end, dry_run);
        end++;
    }
    if (config[end] != ")") {
        throw ParseError(end);
    }

    if (dry_run)
        return 0;
    else
        return new IPCMaxHeuristic(common_options, heuristics_);
}

static ScalarEvaluatorPlugin plugin("max", create);
