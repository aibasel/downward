#include "lm_cut_heuristic.h"

#include "globals.h"
#include "operator.h"
#include "option_parser.h"
#include "plugin.h"
#include "state.h"

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <vector>
using namespace std;



// construction and destruction
LandmarkCutHeuristic::LandmarkCutHeuristic(const Options &opts)
    : Heuristic(opts) {
    num_propositions = 2; // artificial goal and artificial precondition
}

LandmarkCutHeuristic::~LandmarkCutHeuristic() {
}

// initialization
void LandmarkCutHeuristic::initialize() {
    cout << "Initializing landmark cut heuristic..." << endl;

    ::verify_no_axioms_no_conditional_effects();

    // Build propositions.
    assert(num_propositions == 2);
    propositions.resize(g_variable_domain.size());
    for (int var = 0; var < g_variable_domain.size(); var++) {
        for (int value = 0; value < g_variable_domain[var]; value++)
            propositions[var].push_back(RelaxedProposition());
        num_propositions += g_variable_domain[var];
    }

    // Build relaxed operators for operators and axioms.
    for (int i = 0; i < g_operators.size(); i++)
        build_relaxed_operator(g_operators[i]);

    // Simplify relaxed operators.
    // simplify();
    /* TODO: Put this back in and test if it makes sense,
       but only after trying out whether and how much the change to
       unary operators hurts. */

    // Build artificial goal proposition and operator.
    vector<RelaxedProposition *> goal_op_pre, goal_op_eff;
    for (int i = 0; i < g_goal.size(); i++) {
        int var = g_goal[i].first, val = g_goal[i].second;
        RelaxedProposition *goal_prop = &propositions[var][val];
        goal_op_pre.push_back(goal_prop);
    }
    goal_op_eff.push_back(&artificial_goal);
    add_relaxed_operator(goal_op_pre, goal_op_eff, 0, 0);

    // Cross-reference relaxed operators.
    for (int i = 0; i < relaxed_operators.size(); i++) {
        RelaxedOperator *op = &relaxed_operators[i];
        for (int j = 0; j < op->precondition.size(); j++)
            op->precondition[j]->precondition_of.push_back(op);
        for (int j = 0; j < op->effects.size(); j++)
            op->effects[j]->effect_of.push_back(op);
    }
}

void LandmarkCutHeuristic::build_relaxed_operator(const Operator &op) {
    const vector<Prevail> &prevail = op.get_prevail();
    const vector<PrePost> &pre_post = op.get_pre_post();
    vector<RelaxedProposition *> precondition;
    vector<RelaxedProposition *> effects;
    for (int i = 0; i < prevail.size(); i++)
        precondition.push_back(&propositions[prevail[i].var][prevail[i].prev]);
    for (int i = 0; i < pre_post.size(); i++) {
        if (pre_post[i].pre != -1)
            precondition.push_back(&propositions[pre_post[i].var][
                                       pre_post[i].pre]);
        effects.push_back(&propositions[pre_post[i].var][pre_post[i].post]);
    }
    add_relaxed_operator(precondition, effects, &op, get_adjusted_cost(op));
}

void LandmarkCutHeuristic::add_relaxed_operator(
    const vector<RelaxedProposition *> &precondition,
    const vector<RelaxedProposition *> &effects,
    const Operator *op, int base_cost) {
    RelaxedOperator relaxed_op(precondition, effects, op, base_cost);
    if (relaxed_op.precondition.empty())
        relaxed_op.precondition.push_back(&artificial_precondition);
    relaxed_operators.push_back(relaxed_op);
}

// heuristic computation
void LandmarkCutHeuristic::setup_exploration_queue() {
    priority_queue.clear();

    for (int var = 0; var < propositions.size(); var++) {
        for (int value = 0; value < propositions[var].size(); value++) {
            RelaxedProposition &prop = propositions[var][value];
            prop.status = UNREACHED;
        }
    }

    artificial_goal.status = UNREACHED;
    artificial_precondition.status = UNREACHED;

    for (int i = 0; i < relaxed_operators.size(); i++) {
        RelaxedOperator &op = relaxed_operators[i];
        op.unsatisfied_preconditions = op.precondition.size();
        op.h_max_supporter = 0;
        op.h_max_supporter_cost = numeric_limits<int>::max();
    }
}

void LandmarkCutHeuristic::setup_exploration_queue_state(const State &state) {
    for (int var = 0; var < propositions.size(); var++) {
        RelaxedProposition *init_prop = &propositions[var][state[var]];
        enqueue_if_necessary(init_prop, 0);
    }
    enqueue_if_necessary(&artificial_precondition, 0);
}

void LandmarkCutHeuristic::first_exploration(const State &state) {
    assert(priority_queue.empty());
    setup_exploration_queue();
    setup_exploration_queue_state(state);
    while (!priority_queue.empty()) {
        pair<int, RelaxedProposition *> top_pair = priority_queue.pop();
        int popped_cost = top_pair.first;
        RelaxedProposition *prop = top_pair.second;
        int prop_cost = prop->h_max_cost;
        assert(prop_cost <= popped_cost);
        if (prop_cost < popped_cost)
            continue;
        const vector<RelaxedOperator *> &triggered_operators =
            prop->precondition_of;
        for (int i = 0; i < triggered_operators.size(); i++) {
            RelaxedOperator *relaxed_op = triggered_operators[i];
            relaxed_op->unsatisfied_preconditions--;
            assert(relaxed_op->unsatisfied_preconditions >= 0);
            if (relaxed_op->unsatisfied_preconditions == 0) {
                relaxed_op->h_max_supporter = prop;
                relaxed_op->h_max_supporter_cost = prop_cost;
                int target_cost = prop_cost + relaxed_op->cost;
                for (int j = 0; j < relaxed_op->effects.size(); j++) {
                    RelaxedProposition *effect = relaxed_op->effects[j];
                    enqueue_if_necessary(effect, target_cost);
                }
            }
        }
    }
}

void LandmarkCutHeuristic::first_exploration_incremental(
    vector<RelaxedOperator *> &cut) {
    assert(priority_queue.empty());
    /* We pretend that this queue has had as many pushes already as we
       have propositions to avoid switching from bucket-based to
       heap-based too aggressively. This should prevent ever switching
       to heap-based in problems where action costs are at most 1.
    */
    priority_queue.add_virtual_pushes(num_propositions);
    for (int i = 0; i < cut.size(); i++) {
        RelaxedOperator *relaxed_op = cut[i];
        int cost = relaxed_op->h_max_supporter_cost + relaxed_op->cost;
        for (int j = 0; j < relaxed_op->effects.size(); j++) {
            RelaxedProposition *effect = relaxed_op->effects[j];
            enqueue_if_necessary(effect, cost);
        }
    }
    while (!priority_queue.empty()) {
        pair<int, RelaxedProposition *> top_pair = priority_queue.pop();
        int popped_cost = top_pair.first;
        RelaxedProposition *prop = top_pair.second;
        int prop_cost = prop->h_max_cost;
        assert(prop_cost <= popped_cost);
        if (prop_cost < popped_cost)
            continue;
        const vector<RelaxedOperator *> &triggered_operators =
            prop->precondition_of;
        for (int i = 0; i < triggered_operators.size(); i++) {
            RelaxedOperator *relaxed_op = triggered_operators[i];
            if (relaxed_op->h_max_supporter == prop) {
                int old_supp_cost = relaxed_op->h_max_supporter_cost;
                if (old_supp_cost > prop_cost) {
                    relaxed_op->update_h_max_supporter();
                    int new_supp_cost = relaxed_op->h_max_supporter_cost;
                    if (new_supp_cost != old_supp_cost) {
                        // This operator has become cheaper.
                        assert(new_supp_cost < old_supp_cost);
                        int target_cost = new_supp_cost + relaxed_op->cost;
                        for (int j = 0; j < relaxed_op->effects.size(); j++) {
                            RelaxedProposition *effect = relaxed_op->effects[j];
                            enqueue_if_necessary(effect, target_cost);
                        }
                    }
                }
            }
        }
    }
}

void LandmarkCutHeuristic::second_exploration(
    const State &state, vector<RelaxedProposition *> &second_exploration_queue, vector<RelaxedOperator *> &cut) {
    assert(second_exploration_queue.empty());
    assert(cut.empty());

    artificial_precondition.status = BEFORE_GOAL_ZONE;
    second_exploration_queue.push_back(&artificial_precondition);

    for (int var = 0; var < propositions.size(); var++) {
        RelaxedProposition *init_prop = &propositions[var][state[var]];
        init_prop->status = BEFORE_GOAL_ZONE;
        second_exploration_queue.push_back(init_prop);
    }

    while (!second_exploration_queue.empty()) {
        RelaxedProposition *prop = second_exploration_queue.back();
        second_exploration_queue.pop_back();
        const vector<RelaxedOperator *> &triggered_operators =
            prop->precondition_of;
        for (int i = 0; i < triggered_operators.size(); i++) {
            RelaxedOperator *relaxed_op = triggered_operators[i];
            if (relaxed_op->h_max_supporter == prop) {
                bool reached_goal_zone = false;
                for (int j = 0; j < relaxed_op->effects.size(); j++) {
                    RelaxedProposition *effect = relaxed_op->effects[j];
                    if (effect->status == GOAL_ZONE) {
                        assert(relaxed_op->cost > 0);
                        reached_goal_zone = true;
                        cut.push_back(relaxed_op);
                        break;
                    }
                }
                if (!reached_goal_zone) {
                    for (int j = 0; j < relaxed_op->effects.size(); j++) {
                        RelaxedProposition *effect = relaxed_op->effects[j];
                        if (effect->status != BEFORE_GOAL_ZONE) {
                            assert(effect->status == REACHED);
                            effect->status = BEFORE_GOAL_ZONE;
                            second_exploration_queue.push_back(effect);
                        }
                    }
                }
            }
        }
    }
}

void LandmarkCutHeuristic::mark_goal_plateau(RelaxedProposition *subgoal) {
    // NOTE: subgoal can be null if we got here via recursion through
    // a zero-cost action that is relaxed unreachable. (This can only
    // happen in domains which have zero-cost actions to start with.)
    // For example, this happens in pegsol-strips #01.
    if (subgoal && subgoal->status != GOAL_ZONE) {
        subgoal->status = GOAL_ZONE;
        const vector<RelaxedOperator *> &effect_of = subgoal->effect_of;
        for (int i = 0; i < effect_of.size(); i++)
            if (effect_of[i]->cost == 0)
                mark_goal_plateau(effect_of[i]->h_max_supporter);
    }
}

void LandmarkCutHeuristic::validate_h_max() const {
#ifndef NDEBUG
    // Using conditional compilation to avoid complaints about unused
    // variables when using NDEBUG. This whole code does nothing useful
    // when assertions are switched off anyway.
    for (int i = 0; i < relaxed_operators.size(); i++) {
        const RelaxedOperator *op = &relaxed_operators[i];
        const vector<RelaxedProposition *> &prec = op->precondition;
        if (op->unsatisfied_preconditions) {
            bool reachable = true;
            for (int j = 0; j < prec.size(); j++) {
                if (prec[j]->status == UNREACHED) {
                    reachable = false;
                    break;
                }
            }
            assert(!reachable);
            assert(!op->h_max_supporter);
        } else {
            assert(op->h_max_supporter);
            int h_max_cost = op->h_max_supporter_cost;
            assert(h_max_cost == op->h_max_supporter->h_max_cost);
            for (int j = 0; j < prec.size(); j++) {
                assert(prec[j]->status != UNREACHED);
                assert(prec[j]->h_max_cost <= h_max_cost);
            }
        }
    }
#endif
}

int LandmarkCutHeuristic::compute_heuristic(const State &state) {
    // TODO: Possibly put back in some kind of preferred operator mechanism.
    for (int i = 0; i < relaxed_operators.size(); i++) {
        RelaxedOperator &op = relaxed_operators[i];
        op.cost = op.base_cost * COST_MULTIPLIER;
    }

    //cout << "*" << flush;
    int total_cost = 0;

    // The following two variables could be declared inside the loop
    // ("second_exploration_queue" even inside second_exploration),
    // but having them here saves reallocations and hence provides a
    // measurable speed boost.
    vector<RelaxedOperator *> cut;
    vector<RelaxedProposition *> second_exploration_queue;
    first_exploration(state);
    // validate_h_max();  // too expensive to use even in regular debug mode
    if (artificial_goal.status == UNREACHED)
        return DEAD_END;

    int num_iterations = 0;
    while (artificial_goal.h_max_cost != 0) {
        num_iterations++;
        //cout << "h_max = " << artificial_goal.h_max_cost << "..." << endl;
        //cout << "total_cost = " << total_cost << "..." << endl;
        mark_goal_plateau(&artificial_goal);
        assert(cut.empty());
        second_exploration(state, second_exploration_queue, cut);
        assert(!cut.empty());
        int cut_cost = numeric_limits<int>::max();
        for (int i = 0; i < cut.size(); i++) {
            cut_cost = min(cut_cost, cut[i]->cost);
            if (COST_MULTIPLIER > 1) {
                /* We're using this "if" here because COST_MULTIPLIER
                   is currently a global constant and usually 1, which
                   allows the optimizer to get rid of this additional
                   minimization (which is always correct, but not
                   necessary if COST_MULTIPLIER == 1.

                   If COST_MULTIPLIER turns into an option, this code
                   should be changed. I would assume that the savings
                   by the "if" are negligible anyway, but this should
                   be tested.

                   The whole cut cost computation could also be made
                   more efficient in the unit-cost case, where all
                   cuts have cost 1 and the cost decrement could be
                   moved directly to the place where the actions for
                   the cut are collected; indeed, we would not need to
                   collect the cut in a vector at all. But again, I
                   doubt this would have a huge impact, and it would
                   only be applicable in the unit-cost (or zero- and
                   unit-cost) case.
                */
                cut_cost = min(cut_cost, cut[i]->base_cost);
            }
        }
        for (int i = 0; i < cut.size(); i++)
            cut[i]->cost -= cut_cost;
        //cout << "{" << cut_cost << "}" << flush;
        total_cost += cut_cost;

        first_exploration_incremental(cut);
        // validate_h_max();  // too expensive to use even in regular debug mode
        // TODO: Need better name for all explorations; e.g. this could
        //       be "recompute_h_max"; second_exploration could be
        //       "mark_zones" or whatever.
        cut.clear();

        // TODO: Make this more efficient. For example, we can use
        //       a round-dependent counter for GOAL_ZONE and BEFORE_GOAL_ZONE,
        //       or something based on total_cost, in which case we don't
        //       need a per-round reinitialization.
        for (int var = 0; var < propositions.size(); var++) {
            for (int value = 0; value < propositions[var].size(); value++) {
                RelaxedProposition &prop = propositions[var][value];
                if (prop.status == GOAL_ZONE || prop.status == BEFORE_GOAL_ZONE)
                    prop.status = REACHED;
            }
        }
        artificial_goal.status = REACHED;
        artificial_precondition.status = REACHED;
    }
    //cout << "[" << total_cost << "]" << flush;
    //cout << "**************************" << endl;
    return (total_cost + COST_MULTIPLIER - 1) / COST_MULTIPLIER;
}

/* TODO:
   It looks like the change in r3638 reduced the quality of the heuristic
   a bit (at least a preliminary glance at Elevators-03 suggests that).
   The only arbitrary aspect is the tie-breaking policy in choosing h_max
   supporters, so maybe that is adversely affected by the incremental
   procedure? In that case, maybe we should play around with this a bit,
   e.g. use a different tie-breaking rule in every round to spread out the
   values a bit.
 */

static Heuristic *_parse(OptionParser &parser) {
    parser.document_synopsis("Landmark-cut heuristic", "");
    parser.document_language_support("action costs", "supported");
    parser.document_language_support("conditional effects", "not supported");
    parser.document_language_support("axioms", "not supported");
    parser.document_property("admissible", "yes");
    parser.document_property("consistent", "no");
    parser.document_property("safe", "yes");
    parser.document_property("preferred operators", "no");

    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return 0;
    else
        return new LandmarkCutHeuristic(opts);
}


static Plugin<Heuristic> _plugin("lmcut", _parse);
