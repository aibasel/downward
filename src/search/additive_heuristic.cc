#include "additive_heuristic.h"

#include "globals.h"
#include "operator.h"
#include "option_parser.h"
#include "plugin.h"
#include "state.h"

#include <cassert>
#include <vector>
using namespace std;

#include <ext/hash_map>
using namespace __gnu_cxx;


static ScalarEvaluatorPlugin additive_heuristic_plugin(
    "add", AdditiveHeuristic::create);


// construction and destruction
AdditiveHeuristic::AdditiveHeuristic(HeuristicOptions &options)
 : RelaxationHeuristic(options) {
}

AdditiveHeuristic::~AdditiveHeuristic() {
}

// initialization
void AdditiveHeuristic::initialize() {
    cout << "Initializing additive heuristic..." << endl;
    RelaxationHeuristic::initialize();
}

// heuristic computation
void AdditiveHeuristic::setup_exploration_queue() {
    reachable_queue.clear();

    for (int var = 0; var < propositions.size(); var++) {
        for (int value = 0; value < propositions[var].size(); value++) {
            Proposition &prop = propositions[var][value];
            prop.h_add_cost = -1;
            prop.marked = false;
        }
    }

    // Deal with operators and axioms without preconditions.
    for (int i = 0; i < unary_operators.size(); i++) {
        UnaryOperator &op = unary_operators[i];
        op.unsatisfied_preconditions = op.precondition.size();
        op.h_add_cost = op.base_cost; // will be increased by precondition costs

        if (op.unsatisfied_preconditions == 0)
            enqueue_if_necessary(op.effect, op.base_cost, &op);
    }
}

void AdditiveHeuristic::setup_exploration_queue_state(const State &state) {
    for (int var = 0; var < propositions.size(); var++) {
        Proposition *init_prop = &propositions[var][state[var]];
        enqueue_if_necessary(init_prop, 0, 0);
    }
}

void AdditiveHeuristic::relaxed_exploration() {
    int unsolved_goals = goal_propositions.size();
    for (int distance = 0; distance < reachable_queue.size(); distance++) {
        for (;;) {
            Bucket &bucket = reachable_queue[distance];
            // NOTE: Cannot set "bucket" outside the loop because the
            //       reference can change if reachable_queue is
            //       resized.
            if (bucket.empty())
                break;
            Proposition *prop = bucket.back();
            bucket.pop_back();
            int prop_cost = prop->h_add_cost;
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
                unary_op->h_add_cost += prop_cost;
                assert(unary_op->unsatisfied_preconditions >= 0);
                if (unary_op->unsatisfied_preconditions == 0)
                    enqueue_if_necessary(unary_op->effect,
                                         unary_op->h_add_cost, unary_op);
            }
        }
    }
}

void AdditiveHeuristic::mark_preferred_operators(Proposition *goal) {
    if (!goal->marked) { // Only consider each subgoal once.
        goal->marked = true;
        UnaryOperator *unary_op = goal->reached_by;
        if (unary_op) { // We have not yet chained back to a start node.
            for (int i = 0; i < unary_op->precondition.size(); i++)
                mark_preferred_operators(unary_op->precondition[i]);
            if (unary_op->h_add_cost == unary_op->base_cost) {
                // This may be applicable in the current state.
                //
                // TODO: This is no longer a sure-fire way to test
                // applicability with zero-cost actions!
                // Should we test for applicability directly, or should
                // we make it so that operators that are not applicable
                // can safely be marked as preferred?
                const Operator *op = unary_op->op;
                if (!op->is_axiom())
                    set_preferred(op);
            }
        }
    }
}

int AdditiveHeuristic::compute_heuristic(const State &state) {
    setup_exploration_queue();
    setup_exploration_queue_state(state);
    relaxed_exploration();

    int total_cost = 0;
    for (int i = 0; i < goal_propositions.size(); i++) {
        int prop_cost = goal_propositions[i]->h_add_cost;
        if (prop_cost == -1)
            return DEAD_END;
        total_cost += prop_cost;
    }

    for (int i = 0; i < goal_propositions.size(); i++)
        mark_preferred_operators(goal_propositions[i]);

    return total_cost;
}



ScalarEvaluator *AdditiveHeuristic::create(
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
        return new AdditiveHeuristic(common_options);
    }
}
