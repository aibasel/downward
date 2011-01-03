#include "max_heuristic.h"

#include "operator.h"
#include "option_parser.h"
#include "plugin.h"
#include "state.h"

#include <cassert>
#include <vector>
using namespace std;


static ScalarEvaluatorPlugin max_heuristic_plugin(
    "hmax", HSPMaxHeuristic::create);


/*
  TODO: At the time of this writing, this shares huge amounts of code
        with h^add, and the two should be refactored so that the
        common code is only included once, in so far as this is
        possible without sacrificing run-time. We may want to avoid
        virtual calls in the inner-most loops; maybe a templated
        strategy pattern is an option. Right now, the only differences
        to the h^add code are the use of max() instead of add() and
        the lack of preferred operator support (but we might actually
        reintroduce that if it doesn't hurt performance too much).
 */

// construction and destruction
HSPMaxHeuristic::HSPMaxHeuristic(const HeuristicOptions &options)
    : RelaxationHeuristic(options) {
}

HSPMaxHeuristic::~HSPMaxHeuristic() {
}

// initialization
void HSPMaxHeuristic::initialize() {
    cout << "Initializing HSP max heuristic..." << endl;
    RelaxationHeuristic::initialize();
}

// heuristic computation
void HSPMaxHeuristic::setup_exploration_queue() {
    queue.clear();

    for (int var = 0; var < propositions.size(); var++) {
        for (int value = 0; value < propositions[var].size(); value++) {
            Proposition &prop = propositions[var][value];
            prop.h_max_cost = -1;
        }
    }

    // Deal with operators and axioms without preconditions.
    for (int i = 0; i < unary_operators.size(); i++) {
        UnaryOperator &op = unary_operators[i];
        op.unsatisfied_preconditions = op.precondition.size();
        op.h_max_cost = op.base_cost; // will be increased by precondition costs

        if (op.unsatisfied_preconditions == 0)
            enqueue_if_necessary(op.effect, op.base_cost);
    }
}

void HSPMaxHeuristic::setup_exploration_queue_state(const State &state) {
    for (int var = 0; var < propositions.size(); var++) {
        Proposition *init_prop = &propositions[var][state[var]];
        enqueue_if_necessary(init_prop, 0);
    }
}

void HSPMaxHeuristic::relaxed_exploration() {
    int unsolved_goals = goal_propositions.size();
    while (!queue.empty()) {
        pair<int, Proposition *> top_pair = queue.pop();
        int distance = top_pair.first;
        Proposition *prop = top_pair.second;
        int prop_cost = prop->h_max_cost;
        assert(prop_cost <= distance);
        if (prop_cost < distance)
            continue;
        if (prop->is_goal && --unsolved_goals == 0)
            return;
        const vector<UnaryOperator *> &triggered_operators =
            prop->precondition_of;
        for (int i = 0; i < triggered_operators.size(); i++) {
            UnaryOperator *unary_op = triggered_operators[i];
            unary_op->unsatisfied_preconditions--;
            unary_op->h_max_cost = max(unary_op->h_max_cost,
                                       unary_op->base_cost + prop_cost);
            assert(unary_op->unsatisfied_preconditions >= 0);
            if (unary_op->unsatisfied_preconditions == 0)
                enqueue_if_necessary(unary_op->effect, unary_op->h_max_cost);
        }
    }
}

int HSPMaxHeuristic::compute_heuristic(const State &state) {
    setup_exploration_queue();
    setup_exploration_queue_state(state);
    relaxed_exploration();

    int total_cost = 0;
    for (int i = 0; i < goal_propositions.size(); i++) {
        int prop_cost = goal_propositions[i]->h_max_cost;
        if (prop_cost == -1)
            return DEAD_END;
        total_cost = max(total_cost, prop_cost);
    }

    return total_cost;
}

ScalarEvaluator *HSPMaxHeuristic::create(
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
        return new HSPMaxHeuristic(common_options);
    }
}
