#include "ff_heuristic.h"

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
  NOTE: Our implementation of h_add is actually a "poor man's h_add".
  Propositions which were already popped from the queue are not
  reevaluated. Since propositions are ordered by a queue, not by a heap,
  this means that some values might end up too high (which actually happens).
  Maybe it would pay off to use a proper heap, maybe not.
  More experiments are needed. In any case, this is comparatively slooow.
  Are simplifications possible?

  One possible way of doing this is grouping similar unary operators.
  (E.g. in Grid, the "hand status" aspects of pickup-and-loose and pickup
  don't actually need to define a precondition on the current status of the
  hand because *all* possible values are allowed.)
*/


static ScalarEvaluatorPlugin ff_heuristic_plugin("ff", FFHeuristic::create);


// construction and destruction
FFHeuristic::FFHeuristic(HeuristicOptions &options):
    RelaxationHeuristic(options)
{
    reachable_queue_start = 0;
    reachable_queue_read_pos = 0;
    reachable_queue_write_pos = 0;
}

FFHeuristic::~FFHeuristic() {
    delete[] reachable_queue_start;
}

// initialization
void FFHeuristic::initialize() {
    cout << "Initializing FF heuristic..." << endl;
    RelaxationHeuristic::initialize();

    // Allocate reachability queue.
    int total_proposition_no = 0;
    for (int i = 0; i < propositions.size(); i++)
        total_proposition_no += propositions[i].size();
    reachable_queue_start = new Proposition *[total_proposition_no];
}

// heuristic computation
void FFHeuristic::setup_exploration_queue() {
    reachable_queue_read_pos = reachable_queue_start;
    reachable_queue_write_pos = reachable_queue_start;

    for (int var = 0; var < propositions.size(); var++) {
        for (int value = 0; value < propositions[var].size(); value++) {
            Proposition &prop = propositions[var][value];
            prop.h_add_cost = -1;
        }
    }

    // Deal with operators and axioms without preconditions.
    for (int i = 0; i < unary_operators.size(); i++) {
        UnaryOperator &op = unary_operators[i];
        op.unsatisfied_preconditions = op.precondition.size();
        op.h_add_cost = op.base_cost; // will be increased by precondition costs

        if (op.unsatisfied_preconditions == 0) {
            if (op.effect->h_add_cost == -1) {
                *reachable_queue_write_pos++ = op.effect;
                op.effect->h_add_cost = op.base_cost;
                op.effect->reached_by = &op;
            } else if (op.effect->h_add_cost > op.base_cost) {
                op.effect->h_add_cost = op.base_cost;
                op.effect->reached_by = &op;
            }
        }
    }
}

void FFHeuristic::setup_exploration_queue_state(const State &state) {
    for (int var = 0; var < propositions.size(); var++) {
        Proposition *init_prop = &propositions[var][state[var]];
        if (init_prop->h_add_cost == -1)
            *reachable_queue_write_pos++ = init_prop;
        init_prop->h_add_cost = 0;
        init_prop->reached_by = 0; // only needed for FF heuristic
    }
}

void FFHeuristic::relaxed_exploration() {
    int unsolved_goals = goal_propositions.size();
    while (reachable_queue_read_pos != reachable_queue_write_pos) {
        Proposition *prop = *reachable_queue_read_pos++;
        int prop_cost = prop->h_add_cost;
        if (prop->is_goal && --unsolved_goals == 0)
            break;
        const vector<UnaryOperator *> &triggered_operators = prop->precondition_of;
        for (int i = 0; i < triggered_operators.size(); i++) {
            UnaryOperator *unary_op = triggered_operators[i];
            unary_op->unsatisfied_preconditions--;
            unary_op->h_add_cost += prop_cost;
            assert(unary_op->unsatisfied_preconditions >= 0);
            if (unary_op->unsatisfied_preconditions == 0) {
                Proposition *effect = unary_op->effect;
                if (effect->h_add_cost == -1) {
                    // Proposition reached for the first time: put into queue
                    effect->h_add_cost = unary_op->h_add_cost;
                    effect->reached_by = unary_op;
                    *reachable_queue_write_pos++ = effect;
                } else if (unary_op->h_add_cost < effect->h_add_cost) {
                    effect->h_add_cost = unary_op->h_add_cost;
                    effect->reached_by = unary_op;
                }
            }
        }
    }
}

int FFHeuristic::compute_hsp_add_heuristic() {
    relaxed_exploration();

    int total_cost = 0;
    for (int i = 0; i < goal_propositions.size(); i++) {
        int prop_cost = goal_propositions[i]->h_add_cost;
        if (prop_cost == -1)
            return DEAD_END;
        total_cost += prop_cost;
    }
    return total_cost;
}

int FFHeuristic::compute_ff_heuristic() {
    int h_add_heuristic = compute_hsp_add_heuristic();
    if (h_add_heuristic == DEAD_END) {
        return DEAD_END;
    } else {
        RelaxedPlan relaxed_plan;
        relaxed_plan.resize(2 * h_add_heuristic);
        // Collecting the relaxed plan also marks helpful actions as preferred.
        for (int i = 0; i < goal_propositions.size(); i++)
            collect_relaxed_plan(goal_propositions[i], relaxed_plan);
        return relaxed_plan.size();
    }
}

void FFHeuristic::collect_relaxed_plan(Proposition *goal,
                                       RelaxedPlan &relaxed_plan) {
    UnaryOperator *unary_op = goal->reached_by;
    if (unary_op) { // We have not yet chained back to a start node.
        for (int i = 0; i < unary_op->precondition.size(); i++)
            collect_relaxed_plan(unary_op->precondition[i], relaxed_plan);
        const Operator *op = unary_op->op;
        // TODO: we should actually not add axioms to the relaxed plan.
        bool added_to_relaxed_plan = relaxed_plan.insert(op).second;
        if (added_to_relaxed_plan
            && unary_op->h_add_cost == unary_op->base_cost
            && !op->is_axiom()) {
            set_preferred(op); // This is a helpful action.
        }
    }
}

int FFHeuristic::compute_heuristic(const State &state) {
    setup_exploration_queue();
    setup_exploration_queue_state(state);
    return compute_ff_heuristic();
}


ScalarEvaluator *FFHeuristic::create(
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
        return new FFHeuristic(common_options);
    }
}

