#include "reg_max_heuristic.h"

#include <limits>

#include "option_parser.h"
#include "globals.h"
#include "plugin.h"

static ScalarEvaluatorPlugin reg_max_heuristic_plugin(
    "max", RegMaxHeuristic::create);


RegMaxHeuristic::RegMaxHeuristic(const HeuristicOptions &options, const std::vector<Heuristic *> &evals)
    : Heuristic(options), evaluators(evals) {
}

RegMaxHeuristic::~RegMaxHeuristic() {
}

int RegMaxHeuristic::compute_heuristic(const State &state) {
    dead_end = false;
    dead_end_reliable = false;
    value = 0;
    for (unsigned int i = 0; i < evaluators.size(); i++) {
        evaluators[i]->evaluate(state);

        // check for dead end
        if (evaluators[i]->is_dead_end()) {
            value = std::numeric_limits<int>::max();
            dead_end = true;
            if (evaluators[i]->dead_end_is_reliable()) {
                dead_end_reliable = true;
                value = -1;
                break;
            }
        }

        if (evaluators[i]->get_value() > value)
            value = evaluators[i]->get_value();
    }
    return value;
}

bool RegMaxHeuristic::reach_state(const State &parent_state, const Operator &op,
                                  const State &state) {
    int ret = false;
    int val;
    for (int i = 0; i < evaluators.size(); i++) {
        val = evaluators[i]->reach_state(parent_state, op, state);
        ret = ret || val;
    }
    return ret;
}

ScalarEvaluator *RegMaxHeuristic::create(const std::vector<string> &config, int start, int &end, bool dry_run) {
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
        return new RegMaxHeuristic(common_options, heuristics_);
}
