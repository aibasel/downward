#ifndef FF_HEURISTIC_H
#define FF_HEURISTIC_H

#include "relaxation_heuristic.h"

#include <ext/hash_set>

struct hash_operator_ptr {
    size_t operator()(const Operator *key) const {
        return reinterpret_cast<unsigned long>(key);
    }
};

class FFHeuristic : public RelaxationHeuristic {
    typedef __gnu_cxx::hash_set<const Operator *, hash_operator_ptr> RelaxedPlan;

    Proposition **reachable_queue_start;
    Proposition **reachable_queue_read_pos;
    Proposition **reachable_queue_write_pos;

    void setup_exploration_queue();
    void setup_exploration_queue_state(const State &state);
    void relaxed_exploration();

    void collect_relaxed_plan(Proposition *goal, RelaxedPlan &relaxed_plan);

    int compute_hsp_add_heuristic();
    int compute_ff_heuristic();
protected:
    virtual void initialize();
    virtual int compute_heuristic(const State &state);
public:
    FFHeuristic(const HeuristicOptions &options);
    ~FFHeuristic();
    static ScalarEvaluator *create(const std::vector<std::string> &config,
                                   int start, int &end, bool dry_run);
};

#endif
