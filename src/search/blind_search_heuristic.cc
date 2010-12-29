#include "blind_search_heuristic.h"

#include "globals.h"
#include "option_parser.h"
#include "plugin.h"
#include "state.h"


static ScalarEvaluatorPlugin blind_search_heuristic_plugin(
    "blind", BlindSearchHeuristic::create);


BlindSearchHeuristic::BlindSearchHeuristic(const HeuristicOptions &options)
    : Heuristic(options) {
}

BlindSearchHeuristic::~BlindSearchHeuristic() {
}

void BlindSearchHeuristic::initialize() {
    cout << "Initializing blind search heuristic..." << endl;
}

int BlindSearchHeuristic::compute_heuristic(const State &state) {
    bool is_goal = test_goal(state);

    if (is_goal)
        return 0;
    else
        return g_min_action_cost;

    /*
for(int i = 0; i < g_goal.size(); i++) {
    int var = g_goal[i].first, value = g_goal[i].second;
    if(state[var] != value)
        return 1;
}
return 0;
*/
}

ScalarEvaluator *BlindSearchHeuristic::create(
    const std::vector<string> &config, int start, int &end, bool dry_run) {
    HeuristicOptions common_options;

    if (config.size() > start + 2 && config[start + 1] == "(") {
        end = start + 2;
        if (config[end] != ")") {
            NamedOptionParser option_parser;
            common_options.add_option_to_parser(option_parser);

            option_parser.parse_options(config, end, end, dry_run);
            end++;
        }
        if (config[end] != ")")
            throw ParseError(end);
    } else {
        end = start;
    }

    if (dry_run) {
        return 0;
    } else {
        return new BlindSearchHeuristic(common_options);
    }
}
