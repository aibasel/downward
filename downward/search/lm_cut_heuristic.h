#ifndef LM_CUT_HEURISTIC_H
#define LM_CUT_HEURISTIC_H

#include "heuristic.h"

#include <cassert>
#include <vector>

// TODO: Fix duplication with the other relaxation heuristics.

class Operator;
class State;

class RelaxedProposition;
class RelaxedOperator;

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

const int COST_MULTIPLIER = 1;
// Choose 1 for maximum speed, larger values for possibly better
// heuristic accuracy. Heuristic computation time should increase
// roughly linearly with the multiplier.


struct RelaxedOperator {
    const Operator *op;
    std::vector<RelaxedProposition *> precondition;
    std::vector<RelaxedProposition *> effects;
    int base_cost; // 0 for axioms, 1 for regular operators

    int cost;
    int unsatisfied_preconditions;
    RelaxedProposition *h_max_supporter;
    RelaxedOperator(const std::vector<RelaxedProposition *> &pre,
                    const std::vector<RelaxedProposition *> &eff,
                    const Operator *the_op, int base)
        : op(the_op), precondition(pre), effects(eff), base_cost(base) {
    }
};

struct RelaxedProposition {
    std::vector<RelaxedOperator *> precondition_of;
    std::vector<RelaxedOperator *> effect_of;

    int h_max_cost;
    /* TODO: Also add the rpg depth? The Python implementation used
       this for tie breaking, and it led to better landmark extraction
       than just using the cost. However, the Python implementation
       used a heap for the priority queue whereas we use a bucket
       implementation, which automatically gets a lot of tie-breaking
       by depth anyway (although not complete tie-breaking on depth --
       if we add a proposition from cost/depth (4, 9) with (+1,+1),
       we'll process it before one which is added from cost/depth
       (5,5) with (+0,+1). The disadvantage of using depth is that we
       would need a more complicated open queue implementation -- however,
       in the unit action cost case, we might exploit that we never need
       to keep more than the current and next cost layer in memory, and
       simply use two bucket vectors (for two costs, and arbitrarily many
       depths). See if the init h values degrade compared to Python without
       explicit depth tie-breaking, then decide.
    */
    bool in_excluded_set;

    RelaxedProposition() {
    }
};

class LandmarkCutHeuristic : public Heuristic {
    typedef std::vector<RelaxedProposition *> Bucket;

    std::vector<RelaxedOperator> relaxed_operators;
    std::vector<std::vector<RelaxedProposition> > propositions;
    RelaxedProposition artificial_precondition;
    RelaxedProposition artificial_goal;
    std::vector<Bucket> reachable_queue;

    virtual void initialize();
    virtual int compute_heuristic(const State &state);
    void build_relaxed_operator(const Operator &op);
    void add_relaxed_operator(const std::vector<RelaxedProposition *> &precondition,
                              const std::vector<RelaxedProposition *> &effects,
                              const Operator *op, int base_cost);
    void setup_exploration_queue(bool clear_exclude_set);
    void setup_exploration_queue_state(const State &state);
    void relaxed_exploration(bool first_exploration,
                             std::vector<RelaxedOperator *> &cut);

    void enqueue_if_necessary(RelaxedProposition *prop, int cost) {
        assert(cost >= 0);
        if(prop->h_max_cost == -1 || prop->h_max_cost > cost) {
            prop->h_max_cost = cost;
            if(cost >= reachable_queue.size())
                reachable_queue.resize(cost + 1);
            reachable_queue[cost].push_back(prop);
        }
    }

    void mark_goal_plateau(RelaxedProposition *subgoal);
public:
    LandmarkCutHeuristic(bool use_cache=false);
    virtual ~LandmarkCutHeuristic();
};

#endif
