#ifndef POTENTIALS_POTENTIAL_HEURISTICS_H
#define POTENTIALS_POTENTIAL_HEURISTICS_H

#include "potential_heuristic.h"

#include "../global_state.h"
#include "../heuristic.h"
#include "../option_parser.h"

#include <unordered_map>
#include <vector>


namespace potentials {

enum CoverStrategy {
    RANDOM,
    MAX_COVER
};

struct hash_state_id {
    size_t operator()(const StateID &id) const {
        return id.hash();
    }
};

class PotentialHeuristics: public Heuristic {
    OptimizationFunction optimization_function;
    CoverStrategy cover_strategy;
    int size;
    Options opts;
    int num_samples;
    double max_potential;
    double max_filtering_time;
    double max_covering_time;
    bool debug;
    std::vector<PotentialHeuristic *> heuristics;
    StateID get_max_covering_heuristic(
            const std::unordered_map<StateID, PotentialHeuristic *, hash_state_id> &heuristics,
            const std::vector<GlobalState> &samples,
            const std::unordered_map<StateID, int, hash_state_id> &sample_to_max_h);
    void filter_samples(
            std::vector<GlobalState> &samples,
            std::unordered_map<StateID, int, hash_state_id> &sample_to_max_h,
            PotentialHeuristic &heuristic,
            std::unordered_map<StateID, PotentialHeuristic *, hash_state_id> &single_heuristics) const;
    void cover_samples(
            std::vector<GlobalState> &samples,
            const std::unordered_map<StateID, int, hash_state_id> &sample_to_max_h,
            PotentialHeuristic &heuristic,
            std::unordered_map<StateID, PotentialHeuristic *, hash_state_id> &single_heuristics) const;
    void find_complementary_heuristics();
protected:
    virtual void initialize();
    virtual int compute_heuristic(const GlobalState &state);
public:
    PotentialHeuristics(const Options &options);
    ~PotentialHeuristics();
};

}
#endif
