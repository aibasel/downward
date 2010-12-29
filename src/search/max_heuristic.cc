#include "max_heuristic.h"

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

/*
 NOTE: This implementation does not support axioms. A complete
 implementation could be similiar to the implementation in
 additive_heuristic.cc. For such an implementation, more experiments
 are needed because this could be very slow.
*/


static ScalarEvaluatorPlugin max_heuristic_plugin(
    "hmax", HSPMaxHeuristic::create);


// construction and destruction
HSPMaxHeuristic::HSPMaxHeuristic(const HeuristicOptions &options)
    : RelaxationHeuristic(options) {
    reachable_queue_start = 0;
    reachable_queue_read_pos = 0;
    reachable_queue_write_pos = 0;
}

HSPMaxHeuristic::~HSPMaxHeuristic() {
    delete[] reachable_queue_start;
}

// initialization
void HSPMaxHeuristic::initialize() {
    cout << "Initializing HSP max heuristic..." << endl;

    if (!g_axioms.empty()) {
        cerr << "Heuristic does not support axioms!" << endl
             << "Terminating." << endl;
        exit(1);
    }

    RelaxationHeuristic::initialize();

    // Allocate reachability queue.
    int total_proposition_no = 0;
    for (int i = 0; i < propositions.size(); i++)
        total_proposition_no += propositions[i].size();
    reachable_queue_start = new Proposition *[total_proposition_no];
}

// heuristic computation
void HSPMaxHeuristic::setup_exploration_queue() {
    reachable_queue_read_pos = reachable_queue_start;
    reachable_queue_write_pos = reachable_queue_start;

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
        op.h_max_cost = op.base_cost;

        if (op.unsatisfied_preconditions == 0) {
            int curr_cost = op.effect->h_max_cost;
            if (curr_cost == -1)
                *reachable_queue_write_pos++ = op.effect;
            if (curr_cost == -1 || curr_cost > op.base_cost) {
                op.effect->h_max_cost = op.base_cost;
            }
        }
    }
}

void HSPMaxHeuristic::setup_exploration_queue_state(const State &state) {
    for (int var = 0; var < propositions.size(); var++) {
        Proposition *init_prop = &propositions[var][state[var]];
        if (init_prop->h_add_cost == -1)
            *reachable_queue_write_pos++ = init_prop;
        init_prop->h_max_cost = 0;
    }
}

void HSPMaxHeuristic::relaxed_exploration() {
    int unsolved_goals = goal_propositions.size();
    while (reachable_queue_read_pos != reachable_queue_write_pos) {
        Proposition *prop = *reachable_queue_read_pos++;
        int prop_max_cost = prop->h_max_cost;
        if (prop->is_goal && --unsolved_goals == 0)
            break;
        const vector<UnaryOperator *> &triggered_operators = prop->precondition_of;
        for (int i = 0; i < triggered_operators.size(); i++) {
            UnaryOperator *unary_op = triggered_operators[i];
            unary_op->unsatisfied_preconditions--;
            unary_op->h_max_cost = max(unary_op->h_max_cost,
                                       unary_op->base_cost + prop_max_cost);
            assert(unary_op->unsatisfied_preconditions >= 0);
            if (unary_op->unsatisfied_preconditions == 0) {
                Proposition *effect = unary_op->effect;
                if (effect->h_max_cost == -1) {
                    // Proposition reached for the first time: put into queue
                    effect->h_max_cost = unary_op->h_max_cost;
                    *reachable_queue_write_pos++ = effect;
                } else {
                    // Verify that h_max considers operators in order
                    // of increasing cost -- without some kind of
                    // priority queue, this is only true if all
                    // unary_ops have unit cost, i.e., it can easily
                    // fail for tasks with axioms. We don't have a
                    // proper working implementation of h_max with
                    // axioms currently. (We could simply remove the
                    // assertion, which would give us something that
                    // works, but doesn't give the accurate h_max
                    // values in all cases. Rather like the "poor
                    // man's h_add" referred to in the old FF
                    // heuristic implementation.)
                    assert(unary_op->h_max_cost >= effect->h_max_cost);
                }
            }
        }
    }
}

int HSPMaxHeuristic::compute_heuristic(const State &state) {
    setup_exploration_queue();
    setup_exploration_queue_state(state);
    relaxed_exploration();

    int max_cost = 0;
    for (int i = 0; i < goal_propositions.size(); i++) {
        int prop_cost = goal_propositions[i]->h_max_cost;
        if (prop_cost == -1)
            return DEAD_END;
        max_cost = max(max_cost, prop_cost);
    }
    return max_cost;
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
