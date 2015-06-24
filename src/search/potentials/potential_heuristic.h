#ifndef POTENTIALS_POTENTIAL_HEURISTIC_H
#define POTENTIALS_POTENTIAL_HEURISTIC_H

#include "../global_state.h"
#include "../heuristic.h"
#include "../lp_solver.h"
#include "../state_registry.h"

#include <memory>
#include <vector>

namespace potentials {

enum OptimizationFunction {
    INITIAL_STATE,
    EACH_STATE,
    ALL_STATES_AVG,
    SAMPLES_AVG
};

class PotentialHeuristic: public Heuristic {
    friend class PotentialHeuristics;
    LPSolver *lp_solver;
    OptimizationFunction optimization_function;
    Heuristic *sampling_heuristic;
    double sampling_steps_factor;
    int num_samples;
    double max_potential;
    double max_sampling_time;
    double max_filtering_time;
    bool debug;
    bool feasible_lp_solution;
    int num_cols;
    std::vector<std::vector<int> > lp_var_ids;
    std::vector<std::vector<double> > fact_potential;
    int num_samples_covered;
    bool optimize_potential_for_state(const GlobalState &state);
    bool optimize_potential_for_all_states();
    bool optimize_potential_for_samples(const std::vector<GlobalState> &samples);
    void construct_lp();
    void sample_states(StateRegistry &sample_registry,
                       std::vector<GlobalState> &samples,
                       int num_samples,
                       Heuristic *heuristic);
    void filter_dead_ends(const std::vector<GlobalState> &samples,
                          std::vector<GlobalState> &non_dead_end_states);
    void set_objective_for_state(const GlobalState &state);
    void set_objective_for_states(const std::vector<GlobalState> &states);
    void extract_lp_solution();
    bool solve_lp();
    void release_memory();
    void dump_potentials() const;

protected:
    virtual void initialize();
    virtual int compute_heuristic(const GlobalState &state);

public:
    explicit PotentialHeuristic(const Options &options);
    ~PotentialHeuristic() = default;
};

void add_common_potential_options_to_parser(OptionParser &parser);

}
#endif
