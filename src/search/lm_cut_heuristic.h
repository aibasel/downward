#ifndef LM_CUT_HEURISTIC_H
#define LM_CUT_HEURISTIC_H

#include "heuristic.h"
#include "priority_queue.h"

#include <algorithm>
#include <cassert>
#include <vector>

// TODO: Fix duplication with the other relaxation heuristics.

class Operator;
class State;

class RelaxedProposition;
class RelaxedOperator;

class Options;
/* TODO: Check the impact of using unary relaxed operators instead of
   multi-effect ones.

   Pros:
   * potentially simpler code
   * easy integration of conditional effects

   Cons:
   * potentially worse landmark extraction because unary operators
     for the same SAS+ operator are not tied to each other, and hence
     might choose different hmax supporters, leading to unnecessarily
     many paths in the justification graph. (However, if we use a
     systematic method for choosing hmax supporters, this might be
     a non-issue.)

   It's unclear which approach would be faster. Compiling to unary
   operators should be faster in unary domains because we lose the
   vector overhead. In non-unary domains, we have less overhead, but
   more operators.
*/

enum PropositionStatus {
    UNREACHED = 0,
    REACHED = 1,
    GOAL_ZONE = 2,
    BEFORE_GOAL_ZONE = 3
};

const int COST_MULTIPLIER = 1;
// Choose 1 for maximum speed, larger values for possibly better
// heuristic accuracy. Heuristic computation time should increase
// roughly linearly with the multiplier.

/* TODO: In some very preliminary tests in the IPC-2008 Elevators
   domain (more precisely, on Elevators-01), a larger cost multiplier
   (I tried 10) reduced expansion count quite a bit, although not
   enough to balance the extra runtime. Worth experimenting a bit to
   see the effect, though.
 */

struct RelaxedOperator {
    const Operator *op;
    std::vector<RelaxedProposition *> precondition;
    std::vector<RelaxedProposition *> effects;
    int base_cost; // 0 for axioms, 1 for regular operators

    int cost;
    int unsatisfied_preconditions;
    int h_max_supporter_cost; // h_max_cost of h_max_supporter
    RelaxedProposition *h_max_supporter;
    RelaxedOperator(const std::vector<RelaxedProposition *> &pre,
                    const std::vector<RelaxedProposition *> &eff,
                    const Operator *the_op, int base)
        : op(the_op), precondition(pre), effects(eff), base_cost(base) {
    }

    inline void update_h_max_supporter();
};

struct RelaxedProposition {
    std::vector<RelaxedOperator *> precondition_of;
    std::vector<RelaxedOperator *> effect_of;

    PropositionStatus status;
    int h_max_cost;
    /* TODO: Also add the rpg depth? The Python implementation used
       this for tie breaking, and it led to better landmark extraction
       than just using the cost. However, the Python implementation
       used a heap for the priority queue whereas we use a bucket
       implementation [NOTE: no longer true], which automatically gets
       a lot of tie-breaking by depth anyway (although not complete
       tie-breaking on depth -- if we add a proposition from
       cost/depth (4, 9) with (+1,+1), we'll process it before one
       which is added from cost/depth (5,5) with (+0,+1). The
       disadvantage of using depth is that we would need a more
       complicated open queue implementation -- however, in the unit
       action cost case, we might exploit that we never need to keep
       more than the current and next cost layer in memory, and simply
       use two bucket vectors (for two costs, and arbitrarily many
       depths). See if the init h values degrade compared to Python
       without explicit depth tie-breaking, then decide.
    */

    RelaxedProposition() {
    }
};

class LandmarkCutHeuristic : public Heuristic {
    std::vector<RelaxedOperator> relaxed_operators;
    std::vector<std::vector<RelaxedProposition> > propositions;
    RelaxedProposition artificial_precondition;
    RelaxedProposition artificial_goal;
    int num_propositions;
    AdaptiveQueue<RelaxedProposition *> priority_queue;

    virtual void initialize();
    virtual int compute_heuristic(const State &state);
    void build_relaxed_operator(const Operator &op);
    void add_relaxed_operator(const std::vector<RelaxedProposition *> &precondition,
                              const std::vector<RelaxedProposition *> &effects,
                              const Operator *op, int base_cost);
    void setup_exploration_queue();
    void setup_exploration_queue_state(const State &state);
    void first_exploration(const State &state);
    void first_exploration_incremental(std::vector<RelaxedOperator *> &cut);
    void second_exploration(const State &state, std::vector<RelaxedProposition *> &queue,
                            std::vector<RelaxedOperator *> &cut);

    void enqueue_if_necessary(RelaxedProposition *prop, int cost) {
        assert(cost >= 0);
        if (prop->status == UNREACHED || prop->h_max_cost > cost) {
            prop->status = REACHED;
            prop->h_max_cost = cost;
            priority_queue.push(cost, prop);
        }
    }

    void mark_goal_plateau(RelaxedProposition *subgoal);
    void validate_h_max() const;
public:
    LandmarkCutHeuristic(const Options &opts);
    virtual ~LandmarkCutHeuristic();
};

inline void RelaxedOperator::update_h_max_supporter() {
    assert(!unsatisfied_preconditions);
    for (int i = 0; i < precondition.size(); i++)
        if (precondition[i]->h_max_cost > h_max_supporter->h_max_cost)
            h_max_supporter = precondition[i];
    h_max_supporter_cost = h_max_supporter->h_max_cost;
}

#endif
